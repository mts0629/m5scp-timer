#include <M5StickCPlus.h>

// Timer state
enum State {
  STATE_STOP = 0,
  STATE_RUNNING
};

State state;

// Count in 100 ms
int count;
const int duration_sec = 30;

void setup() {
  M5.begin();

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setRotation(1);

  state = STATE_STOP;
}

void switch_state(void) {
  int text_size;

  switch (state) {
    case STATE_STOP:
      state = STATE_RUNNING;
      count = duration_sec * 10;
      text_size = 5;
      break;
    case STATE_RUNNING:
    default:
      state = STATE_STOP;
      text_size = 3;
      break;
  }

  M5.Lcd.setTextSize(text_size);
  M5.Lcd.fillScreen(BLACK);
}

void loop() {
  M5.update();

  M5.Lcd.setCursor(0, 0);

  if (M5.BtnA.isPressed()) {
    if (state == STATE_STOP) {      
      switch_state();
    }
  }

  if (state == STATE_RUNNING) {
    M5.Lcd.printf("%d.%d", count / 10, count % 10);
  } else {
    M5.Lcd.printf("Timer: %d sec\n", duration_sec);
    M5.Lcd.print("[A] start");
  }
  
  delay(100);

  if (state == STATE_RUNNING) {
    count--;

    if (count == 0) {
      // Print "0.0"
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.printf("%d.%d", count / 10, count % 10);

      // Beep twice
      for (int i = 0; i < 2; i++) {
        M5.Beep.beep();
        delay(200);
        M5.Beep.end();
        delay(100);
      }

      // Wait about 1 sec totally
      delay(400);

      switch_state();
    }
  }
}
