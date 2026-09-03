#include <M5StickCPlus.h>

// Timer state
enum State {
  STATE_STOP = 0,
  STATE_RUNNING
};

State state;

// Count in 100 ms
int count;
int duration_sec = 0;

void setup() {
  M5.begin();

  // Initialize I2C connection to Hat Mini Encoder
  Wire.begin(0, 26, 100000UL);
  delay(10);
  Wire.beginTransmission(0x42);
  Wire.endTransmission(true);
  delay(100);

  // Reset a counter
  Wire.beginTransmission(0x42);
  Wire.write(0x40);
  Wire.write(1);
  Wire.endTransmission(true);
  delay(100);

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setRotation(0);

  state = STATE_STOP;
}

void switch_state(void) {
  switch (state) {
    case STATE_STOP:
      state = STATE_RUNNING;
      count = duration_sec * 10;
      break;
    case STATE_RUNNING:
    default:
      state = STATE_STOP;
      break;
  }

  M5.Lcd.fillScreen(BLACK);
}

void beep(const int ms) {
  M5.Beep.beep();
  delay(ms);
  M5.Beep.end();
}

void loop() {
  M5.update();

  M5.Lcd.setCursor(0, 0);

  if (state == STATE_RUNNING) {
    M5.Lcd.setTextSize(5);
    M5.Lcd.printf("%02d s\n%d", count / 10, count % 10);
  } else {
    Wire.beginTransmission(0x42);
    Wire.write(0x10);
    Wire.endTransmission(true);
    Wire.requestFrom(0x42, 4);
    uint8_t data[4];
    for (int i = 0; i < 4; i++) {
      data[i] = Wire.read();
    }
  
    int val = ((data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0]);

    // Change duration
    if (val > 0) {
      duration_sec++;
    } else if (val < 0) {
      duration_sec--;
    }
    // Rotate for 60 sec
    if (duration_sec > 59) {
      duration_sec = 0;
    } else if (duration_sec < 0) {
      duration_sec = 59;
    }
    
    M5.Lcd.setTextSize(5);
    M5.Lcd.printf("%02d s\n", duration_sec);
    M5.Lcd.setTextSize(3);
    M5.Lcd.print("[A]\nstart\n");
    M5.Lcd.print("[Knob]\nsec\n");

    if (M5.BtnA.isPressed()) {
      if (state == STATE_STOP) {      
        beep(100);

        switch_state();
        return;
      }
    }
  }

  delay(100);

  if (state == STATE_RUNNING) {
    count--;
    if (count < 0) {
      count = 0;
    }

    if (count == 0) {
      // Print "0.0"
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.printf("%02d s\n%d", count / 10, count % 10);

      // Beep twice
      for (int i = 0; i < 2; i++) {
        beep(100);
        delay(50);
      }

      // Wait about 1 sec totally
      delay(700);

      switch_state();
    }
  }
}
