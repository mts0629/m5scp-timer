#include <EEPROM.h>
#include <M5Unified.h>

// Timer state
enum State {
  STATE_CONFIG,
  STATE_RUNNING,
  STATE_STOP
};

static State state;

// Count in ms
static int count;
#define SCALE_SEC 1000

// Start of elapsed count
static unsigned long prev_ms;
// Sum of elapsed count
static int sum_elapsed_ms;

// Duration
static int duration;
#define MAX_DURATION ((99 * 60 + 59) * SCALE_SEC)

// Button state
#define BUTTON_NONE 0
#define BUTTON_PRESSED 1
#define BUTTON_HOLDING 2

// Duration selector
#define SELECT_NONE 0
#define SELECT_MIN 1
#define SELECT_SEC 2
static uint8_t selector;

// Configuration saved address on EEPROM
#define EEPROM_SAVE_ADDR 0

// Time (min/sec)
struct DispTime {
  int min;
  int sec;
};

// Displayed time
static DispTime disp_time = { 0, 0 };
// Configured time
static DispTime cfg_time = { 0, 0 };

int cvt_to_duration(const DispTime cfg) {
  return ((cfg.min * 60) + cfg.sec) * SCALE_SEC;
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  // Initialization
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setRotation(0);
  state = STATE_CONFIG;
  selector = SELECT_NONE;

  // Load saved configuration
  EEPROM.begin(sizeof(DispTime));
  EEPROM.get(EEPROM_SAVE_ADDR, cfg_time);
  duration = cvt_to_duration(cfg_time);
}

void switch_state(const State next_state) {
  switch (next_state) {
    case STATE_RUNNING:
      if (state == STATE_CONFIG) {
        count = duration;
        disp_time = cfg_time;
        sum_elapsed_ms = 0;
      }

      prev_ms = millis();
      state = STATE_RUNNING;
      break;
    case STATE_STOP:
      state = STATE_STOP;
      break;
    default: // STATE_CONFIG
      state = STATE_CONFIG;
      break;
  }

  M5.Lcd.fillScreen(BLACK);
}

void print_time() {
  int fg_color = WHITE;
  // Change fg color
  if (state != STATE_CONFIG) {
    if (count <= (5 * SCALE_SEC)) {
      // Remaining 5 sec: print by red
      fg_color = RED;
    } else if (count <= (10 * SCALE_SEC)) {
      // Remaining 10 sec: print by yellow
      fg_color = YELLOW;
    }
  }
 
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextColor(fg_color, BLACK);
  
  DispTime *t = (state == STATE_CONFIG) ? &cfg_time : &disp_time;

  // Minutes
  M5.Lcd.setTextSize(10);
  if ((state == STATE_CONFIG) && (selector == SELECT_MIN)) {
    // Selected on configuration
    M5.Lcd.setTextColor(YELLOW, BLACK);
  }
  M5.Lcd.printf("%02d", t->min);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(fg_color, BLACK);
  M5.Lcd.print("'");

  // Seconds
  M5.Lcd.setTextSize(10);
  if ((state == STATE_CONFIG) && (selector == SELECT_SEC)) {
    // Selected on configuration
    M5.Lcd.setTextColor(YELLOW, BLACK);
  }
  M5.Lcd.printf("\n%02d", t->sec);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(fg_color, BLACK);
  M5.Lcd.print("\"");
}

void print_state() {
  M5.Lcd.setTextSize(10);
  M5.Lcd.println();
  M5.Lcd.setTextSize(3);
  if (state == STATE_RUNNING) {
    M5.Lcd.print("\nRUNNING");
  } else if (state == STATE_STOP) {
    M5.Lcd.print("\nSTOP");
  } else {
    M5.Lcd.print("\nCONFIG");
  }
}

void change_duration(const int inc_val) {
  int *selected = (selector == SELECT_SEC) ? &(cfg_time.sec) : &(cfg_time.min);
  if (inc_val > 0) {
    (*selected)++;
  } else if (inc_val < 0) {
    (*selected)--;
  }

  // Rotate seconds
  if (cfg_time.sec > 59) {
    cfg_time.sec = 0;
  } else if (cfg_time.sec < 0) {
    cfg_time.sec = 59;
  }
  // Rotate minutes
  if (cfg_time.min > 99) {
    cfg_time.min = 0;
  } else if (cfg_time.min < 0) {
    cfg_time.min = 99;
  }

  duration = cvt_to_duration(cfg_time);
}

void reset_config() {
  cfg_time.min = 0;
  cfg_time.sec = 0;
}

void beep(const int time) {
  constexpr int ms = 100;
  constexpr int duration = 100;
  constexpr int interval = 50;

  for (int i = 0; i < time; i++) {
    M5.Speaker.tone(2000, ms, 0, false);
    delay(duration + interval);
  }
}

void proc_time(const unsigned long start_ms, const unsigned long end_ms) {
  int elapsed = (int)(end_ms - start_ms);
  count -= elapsed;
  sum_elapsed_ms += elapsed;

  if (sum_elapsed_ms >= SCALE_SEC) {
    disp_time.sec--;

    if (disp_time.sec < 0) {
      disp_time.sec = 59;
      disp_time.min--;
    }
    if (disp_time.min < 0) {
      disp_time.min = 0;
    }

    sum_elapsed_ms = (sum_elapsed_ms - SCALE_SEC);
  }
}

void finish_timer() {  
  count = 0;

  // Print zero
  print_time();
  print_state();

  beep(2);

  // Wait 2 sec (wait 300 ms on beep x2)
  delay(1700);
}

bool is_btn_a_pressed() {
  static bool prev_pressed = false;
  static bool pressed = false;

  pressed = M5.BtnA.isPressed();

  // Detect the change: (released -> pressed)
  bool detected = (!prev_pressed && pressed);
  prev_pressed = pressed;

  return detected;
}

int get_btn_a_state() {
  constexpr int hold_duration = SCALE_SEC / 2; // 500 ms
  static bool pressed = false;
  static unsigned long btn_a_press_start = 0;

  unsigned long now;
  if (M5.BtnA.isPressed()) {
    if (!pressed) {
      btn_a_press_start = millis();
      pressed = true;
    }

    now = millis();

    if ((now - btn_a_press_start) > hold_duration) {
      return BUTTON_HOLDING;
    }
  } else {
    if (pressed) {
      pressed = false;
      return BUTTON_PRESSED;
    }
  }

  return BUTTON_NONE;
}

int get_btn_b_state() {
  constexpr int hold_duration = SCALE_SEC / 2; // 500 ms
  static bool pressed = false;
  static unsigned long btn_b_press_start = 0;

  unsigned long now;
  if (M5.BtnB.isPressed()) {
    if (!pressed) {
      btn_b_press_start = millis();
      pressed = true;
    }

    now = millis();

    if ((now - btn_b_press_start) > hold_duration) {
      return BUTTON_HOLDING;
    }
  } else {
    if (pressed) {
      pressed = false;
      return BUTTON_PRESSED;
    }
  }

  return BUTTON_NONE;
}

void run() {
  unsigned long now_ms = millis();
  proc_time(prev_ms, now_ms);

  print_time();
  print_state();

  if (is_btn_a_pressed()) {
    beep(1);
    switch_state(STATE_STOP);
    return;
  }

  if (count <= 0) {
    finish_timer();
    switch_state(STATE_CONFIG);
  }

  prev_ms = now_ms;
}

void stop() {
  print_time();
  print_state();

  if (is_btn_a_pressed()) {
    beep(1);
    switch_state(STATE_RUNNING);
    return;
  }

  if (get_btn_b_state() == BUTTON_PRESSED) {
    beep(2);
    switch_state(STATE_CONFIG);
    return;
  }
}

void save_config() {
  EEPROM.put(EEPROM_SAVE_ADDR, cfg_time);
  EEPROM.commit();
}

void configure() {
  static bool btn_b_holding = false;

  print_time();
  print_state();

  if (get_btn_a_state() == BUTTON_PRESSED) {
    if (selector == SELECT_NONE) {
      beep(1);
      switch_state(STATE_RUNNING);
      return;
    } else {
      change_duration(1);
    }
  } else if (get_btn_a_state() == BUTTON_HOLDING) {
    if (selector != SELECT_NONE) {
      change_duration(1);
    }
  }

  if (get_btn_b_state() == BUTTON_PRESSED) {
    if (!btn_b_holding) {
      beep(1);

      selector = (selector == SELECT_NONE) ? SELECT_MIN :
                 (selector == SELECT_MIN) ? SELECT_SEC :
                 SELECT_NONE;

      if (selector == SELECT_NONE) {
        save_config();
      }
    }
  } else if (get_btn_b_state() == BUTTON_HOLDING) {
    if (!btn_b_holding) {
      beep(2);

      if (selector == SELECT_MIN) {
        cfg_time.min = 0;
      } else if (selector == SELECT_SEC) {
        cfg_time.sec = 0;
      } else {
        reset_config();
      }

      save_config();
      btn_b_holding = true;
    }
  } else { // BUTTON_NONE
    btn_b_holding = false;
  }
}

void loop() {
  M5.update();

  if (state == STATE_RUNNING) {
    run();
  } else if (state == STATE_STOP) {
    stop();
  } else { // STATE_CONFIG
    configure();
  }

  delay(100);
}
