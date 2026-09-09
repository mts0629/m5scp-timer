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

// Count in 100 ms
static int count;
#define SCALE_SEC 10

// Duration
static int duration = 0;
#define MAX_DURATION ((99 * 60 + 59) * SCALE_SEC)

// Duration selector
static uint8_t selector;
#define SELECT_SEC 0
#define SELECT_MIN 1

// Min/sec configuration
static int cfg_min = 0;
static int cfg_sec = 0;

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
      }
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

void print_time(const int count) {
  // Convert the count to time
  int m = count / (60 * SCALE_SEC);
  int rem = count % (60 * SCALE_SEC);
  int s = rem / SCALE_SEC;
  int ms = rem % SCALE_SEC;

  int fg_color = WHITE;
  if (state == STATE_RUNNING) {
    // Remaining 5 sec: print by red
    if (count < (5 * SCALE_SEC)) {
      fg_color = RED;
    } else if (count < (10 * SCALE_SEC)) {
    // Remaining 10 sec: print by yellow
      fg_color = YELLOW;
    }
  }
 
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextColor(fg_color, BLACK);

  // Minutes
  M5.Lcd.setTextSize(9);
  if ((state == STATE_CONFIG) && (selector == SELECT_MIN)) {
    M5.Lcd.setTextColor(YELLOW, BLACK);
  }
  M5.Lcd.printf("%02d", m);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(fg_color, BLACK);
  M5.Lcd.printf("'");

  // Seconds
  M5.Lcd.setTextSize(9);
  if ((state == STATE_CONFIG) && (selector == SELECT_SEC)) {
    M5.Lcd.setTextColor(YELLOW, BLACK);
  }
  M5.Lcd.printf("\n%02d", s);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(fg_color, BLACK);
  M5.Lcd.printf("\"");

  // Milliseconds
  M5.Lcd.setTextSize(9);
  M5.Lcd.printf("\n%d", ms);
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
  static bool pressed = false;
  if (Wire.read() == 0) {
    // Detect push
    if (!pressed) {
      pressed = true;
    }
  } else {
    // Detect release
    if (pressed) {
      pressed = false;
    }
  }

  return pressed;
}

void change_duration(const int inc_val) {
  if (selector == SELECT_SEC) {
    if (inc_val > 0) {
      cfg_sec++;
    } else if (inc_val < 0) {
      cfg_sec--;
    }

    // Rotate seconds
    if (cfg_sec > 59) {
      cfg_sec = 0;
    } else if (cfg_sec < 0) {
      cfg_sec = 59;
    }
  } else if (selector == SELECT_MIN) {
    if (inc_val > 0) {
      cfg_min++;
    } else if (inc_val < 0) {
      cfg_min--;
    }
    
    // Rotate minutes
    if (cfg_min > 99) {
      cfg_min = 0;
    } else if (cfg_min < 0) {
      cfg_min = 99;
    }
  }

  duration = ((cfg_min * 60) + cfg_sec) * SCALE_SEC;
}

void reset_duration() {
  cfg_min = 0;
  cfg_sec = 0;
  duration = 0;
}

void beep(const int ms) {
  M5.Speaker.tone(2000, 100, 0, false);
  delay(ms);
}

void finish_timer() {  
  count = 0;

  // Print zero
  print_time(count);

  // Beep twice
  for (int i = 0; i < 2; i++) {
    beep(100);
    delay(50);
  }

  // Wait about 1 sec totally
  delay(700);
}

void loop() {
  M5.update();

  if (state == STATE_RUNNING) {
    print_time(count);
    
    if (M5.BtnA.isPressed()) {
      beep(100);
      switch_state(STATE_STOP);
      return;
    }

    delay(100);

    count--;

    if (count <= 0) {
      finish_timer();
      switch_state(STATE_CONFIG);
    }
  } else if (state == STATE_STOP) {
    print_time(count);

    delay(100);

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
  } else { // STATE_CONFIG
    if (is_enc_btn_pressed()) {
      selector = (selector == SELECT_SEC) ? SELECT_MIN : SELECT_SEC;
    }

    int val = get_incremental_value();

    change_duration(val);

    print_time(duration);

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

    delay(100);
  }
}
