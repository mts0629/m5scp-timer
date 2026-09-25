#include <EEPROM.h>
#include <M5Unified.h>

// Timer state
enum class State {
    Config,
    Running,
    Stop
};
static State state;

// Count in ms
static int count;
constexpr int SCALE_SEC = 1000;

// Start of elapsed count
static unsigned long prev_ms;
// Sum of elapsed count
static int sum_elapsed_ms;

// Duration
static int duration;
constexpr int MAX_DURATION = ((99 * 60 + 59) * SCALE_SEC);

// Button state
enum class ButtonState {
    None,
    Pressed,
    Holding
};

// Duration selector
enum class Selector {
    None,
    Min,
    Sec
};
static Selector selector;

// Configuration saved address on EEPROM
constexpr int EEPROM_SAVE_ADDR = 0;

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
    state = State::Config;
    selector = Selector::None;

    // Load saved configuration
    EEPROM.begin(sizeof(DispTime));
    EEPROM.get(EEPROM_SAVE_ADDR, cfg_time);
    duration = cvt_to_duration(cfg_time);
}

void switch_state(const State next_state) {
    switch (next_state) {
        case State::Running:
            if (state == State::Config) {
                count = duration;
                disp_time = cfg_time;
                sum_elapsed_ms = 0;
            }

            prev_ms = millis();
            state = State::Running;
            break;
        case State::Stop:
            state = State::Stop;
            break;
        default: // State::Config
            state = State::Config;
            break;
    }

    M5.Lcd.fillScreen(BLACK);
}

void print_time() {
    int fg_color = WHITE;

    // Change fg color
    if (state != State::Config) {
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
    
    DispTime *t = (state == State::Config) ? &cfg_time : &disp_time;

    // Minutes
    M5.Lcd.setTextSize(10);
    if ((state == State::Config) && (selector == Selector::Min)) {
        // Selected on configuration
        M5.Lcd.setTextColor(YELLOW, BLACK);
    }
    M5.Lcd.printf("%02d", t->min);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(fg_color, BLACK);
    M5.Lcd.print("'");

    // Seconds
    M5.Lcd.setTextSize(10);
    if ((state == State::Config) && (selector == Selector::Sec)) {
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
    if (state == State::Running) {
        M5.Lcd.print("\nRUNNING");
    } else if (state == State::Stop) {
        M5.Lcd.print("\nSTOP");
    } else {
        M5.Lcd.print("\nCONFIG");
    }
}

void change_duration(const int inc_val) {
    int *selected = (selector == Selector::Sec) ? &(cfg_time.sec)
                                                : &(cfg_time.min);
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
    constexpr int MS = 100;
    constexpr int DURATION = 100;
    constexpr int INTERVAL = 50;

    for (int i = 0; i < time; i++) {
        M5.Speaker.tone(2000, MS, 0, false);
        delay(DURATION + INTERVAL);
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

ButtonState get_btn_a_state() {
    constexpr int HOLD_DURATION = SCALE_SEC / 2; // 500 ms
    static bool pressed = false;
    static unsigned long btn_a_press_start = 0;

    unsigned long now;
    if (M5.BtnA.isPressed()) {
        if (!pressed) {
            btn_a_press_start = millis();
            pressed = true;
        }

        now = millis();

        if ((now - btn_a_press_start) > HOLD_DURATION) {
            return ButtonState::Holding;
        }
    } else {
        if (pressed) {
            pressed = false;
            return ButtonState::Pressed;
        }
    }

    return ButtonState::None;
}

ButtonState get_btn_b_state() {
    constexpr int HOLD_DURATION = SCALE_SEC / 2; // 500 ms
    static bool pressed = false;
    static unsigned long btn_b_press_start = 0;

    unsigned long now;
    if (M5.BtnB.isPressed()) {
        if (!pressed) {
            btn_b_press_start = millis();
            pressed = true;
        }

        now = millis();

        if ((now - btn_b_press_start) > HOLD_DURATION) {
            return ButtonState::Holding;
        }
    } else {
        if (pressed) {
            pressed = false;
            return ButtonState::Pressed;
        }
    }

    return ButtonState::None;
}

void run() {
    unsigned long now_ms = millis();
    proc_time(prev_ms, now_ms);

    print_time();
    print_state();

    if (is_btn_a_pressed()) {
        beep(1);
        switch_state(State::Stop);
        return;
    }

    if (count <= 0) {
        finish_timer();
        switch_state(State::Config);
    }

    prev_ms = now_ms;
}

void stop() {
    print_time();
    print_state();

    if (is_btn_a_pressed()) {
        beep(1);
        switch_state(State::Running);
        return;
    }

    if (get_btn_b_state() == ButtonState::Pressed) {
        beep(2);
        switch_state(State::Config);
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

    auto btn_a_state = get_btn_a_state();
    if (btn_a_state == ButtonState::Pressed) {
        if (selector == Selector::None) {
            beep(1);
            switch_state(State::Running);
            return;
        } else {
            change_duration(1);
        }
    } else if (btn_a_state == ButtonState::Holding) {
        if (selector != Selector::None) {
            change_duration(1);
        }
    }

    auto btn_b_state = get_btn_b_state();
    if (btn_b_state == ButtonState::Pressed) {
        if (!btn_b_holding) {
            beep(1);

            selector = (selector == Selector::None) ? Selector::Min :
                       (selector == Selector::Min) ? Selector::Sec :
                       Selector::None;

            if (selector == Selector::None) {
                save_config();
            }
        }
    } else if (btn_b_state == ButtonState::Holding) {
        if (!btn_b_holding) {
            beep(2);

            if (selector == Selector::Min) {
                cfg_time.min = 0;
            } else if (selector == Selector::Sec) {
                cfg_time.sec = 0;
            } else {
                reset_config();
            }

            save_config();
            btn_b_holding = true;
        }
    } else { // ButtonState::NONE
        btn_b_holding = false;
    }

    // Checking no operation time
    static bool nop_started = false;
    if ((btn_a_state == ButtonState::None) &&
        (btn_b_state == ButtonState::None)) {
        static unsigned long nop_start_time;
        if (nop_started) {
            auto now = millis();
            // When no operation time continues, power off automatically
            constexpr unsigned long TIMEOUT = 3 * 60 * SCALE_SEC;
            if ((now - nop_start_time) > TIMEOUT) {
                M5.Power.powerOff();
            }
        } else {
            nop_start_time = millis();
            nop_started = true;
        }
    } else {
        nop_started = false;
    }
}

void loop() {
    M5.update();

    if (state == State::Running) {
        run();
    } else if (state == State::Stop) {
        stop();
    } else { // State::Config
        configure();
    }

    delay(100);
}
