#include <Wire.h>
#include <M5Unified.h>

// Hat Mini Encoder C specification
#define ENC_INC_ADDR 0x10
#define ENC_BTN_ADDR 0x20
#define ENC_RST_ADDR 0x40
#define ENC_I2C_ADDR 0x42
#define ENC_PIN_SDA 0
#define ENC_PIN_SCL 26

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
static int duration = 0;
#define MAX_DURATION ((99 * 60 + 59) * SCALE_SEC)

// Duration selector
static uint8_t selector;
#define SELECT_SEC 0
#define SELECT_MIN 1

// Time (min/sec)
struct DispTime {
  int min;
  int sec;
};

// Displayed time
static DispTime disp_time;
// Configured time
static DispTime cfg_time;

void setup() {
  auto cfg = M5.config();

  M5.begin(cfg);

  // Initialize I2C connection to the encoder
  Wire.begin(ENC_PIN_SDA, ENC_PIN_SCL, 100000UL);
  delay(10);
  Wire.beginTransmission(ENC_I2C_ADDR);
  Wire.endTransmission(true);
  delay(100);

  // Reset a counter
  Wire.beginTransmission(ENC_I2C_ADDR);
  Wire.write(ENC_RST_ADDR);
  Wire.write(1);
  Wire.endTransmission(true);
  delay(100);

  // Initialize the screen
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setRotation(0);

  state = STATE_CONFIG;
  selector = SELECT_SEC;
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

int get_incremental_value() {
  Wire.beginTransmission(ENC_I2C_ADDR);
  Wire.write(ENC_INC_ADDR);
  Wire.endTransmission(true);
  Wire.requestFrom(ENC_I2C_ADDR, 4);

  uint8_t data[4];
  for (int i = 0; i < 4; i++) {
    data[i] = Wire.read();
  }
  
  return ((data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0]);
}

bool is_enc_btn_pressed() {
  Wire.beginTransmission(ENC_I2C_ADDR);
  Wire.write(ENC_BTN_ADDR);
  Wire.endTransmission(false);
  Wire.requestFrom(ENC_I2C_ADDR, 1);

  static bool prev_pressed = false;
  static bool pressed = false;
  pressed = (Wire.read() == 0);

  // Detect the change: (released -> pressed)
  bool trigger = (!prev_pressed && pressed);
  prev_pressed = pressed;

  return trigger;
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

  duration = ((cfg_time.min * 60) + cfg_time.sec) * SCALE_SEC;
}

void reset_duration() {
  cfg_time.min = 0;
  cfg_time.sec = 0;
  duration = 0;
}

void beep(const int ms) {
  M5.Speaker.tone(2000, 100, 0, false);
  delay(ms);
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

  // Beep twice
  for (int i = 0; i < 2; i++) {
    beep(100);
    delay(50);
  }

  // Wait 2 sec totally
  delay(1700);
}

void run() {
  unsigned long now_ms = millis();
  proc_time(prev_ms, now_ms);

  print_time();
  print_state();

  if (M5.BtnA.isPressed()) {
    beep(100);
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

  if (M5.BtnA.isPressed()) {
    beep(100);
    switch_state(STATE_RUNNING);
    return;
  }

  if (M5.BtnB.isPressed()) {
    beep(100);
    switch_state(STATE_CONFIG);
    return;
  }
}

void configure() {
  if (is_enc_btn_pressed()) {
    selector = (selector == SELECT_SEC) ? SELECT_MIN : SELECT_SEC;
  }

  int val = get_incremental_value();
  if (val != 0) {
    change_duration(val);
  }

  print_time();
  print_state();

  if (M5.BtnA.isPressed()) {
    beep(100);
    switch_state(STATE_RUNNING);
    return;
  }

  if (M5.BtnB.isPressed()) {
    beep(100);
    reset_duration();
    return;
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
