#include <M5StickCPlus.h>

// Hat Mini Encoder C specification
#define ENC_INC_ADDR 0x10
#define ENC_RST_ADDR 0x40
#define ENC_I2C_ADDR 0x42
#define ENC_PIN_SDA 0
#define ENC_PIN_SCL 26

// Timer state
enum State {
  STATE_STOP = 0,
  STATE_RUNNING
};

static State state;

// Count in 100 ms
static int count;
#define SCALE_SEC 10

// Duration
static int duration = 0;
#define MAX_DURATION ((99 * 60 + 59) * SCALE_SEC)

void setup() {
  M5.begin();

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
  M5.Lcd.setRotation(1);

  state = STATE_STOP;
}

void switch_state() {
  switch (state) {
    case STATE_STOP:
      state = STATE_RUNNING;
      count = duration;
      break;
    case STATE_RUNNING:
    default:
      state = STATE_STOP;
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

  M5.Lcd.setCursor(0, 0);

  // Minutes
  M5.Lcd.setTextSize(10);
  M5.Lcd.printf("%02d", m);
  M5.Lcd.setTextSize(3);
  M5.Lcd.printf("'");

  // Seconds
  M5.Lcd.setTextSize(10);
  M5.Lcd.printf("%02d", s);
  M5.Lcd.setTextSize(3);
  M5.Lcd.printf("\"");

  // Milliseconds
  M5.Lcd.setTextSize(10);
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

void change_duration(const int inc_val) {
  if (inc_val > 0) {
    duration += 1 * SCALE_SEC;
  } else if (inc_val < 0) {
    duration -= 1 * SCALE_SEC;
  }

  // Rotate duration
  if (duration > MAX_DURATION) {
    duration = 0;
  } else if (duration < 0) {
    duration = MAX_DURATION;
  }
}

void beep(const int ms) {
  M5.Beep.beep();
  delay(ms);
  M5.Beep.end();
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

    delay(100);

    count--;

    if (count <= 0) {
      finish_timer();
      switch_state();
    }
  } else {
    int val = get_incremental_value();

    change_duration(val);

    print_time(duration);

    if (M5.BtnA.isPressed()) {
      if (state == STATE_STOP) {
        beep(100);
        switch_state();
        return;
      }
    }

    delay(100);
  }
}
