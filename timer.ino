#include <M5StickCPlus.h>

enum State {
  STATE_STOP = 0,
  STATE_RUNNING
};

const char *messages[] = {
  "Press A to start",
  "Started"
};

State state;
int count;

void setup() {
  M5.begin();

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(4);
  M5.Lcd.setRotation(1);

  state = STATE_STOP;
  count = 0;
}

void loop() {
  M5.update();

  if (M5.BtnA.isPressed()) {
    if (state == STATE_STOP) {
      state = STATE_RUNNING;
      count = 5;
    }
  }

  M5.Lcd.setCursor(0, 0);

  const char *message = messages[state];

  if (state == STATE_RUNNING) {
    M5.Lcd.fillScreen(BLACK);

    M5.Lcd.print(message);

    delay(1000);

    count--;

    if (count == 0) {
      state = STATE_STOP;
    }
  } else {
    M5.Lcd.print(message);
  }
}

