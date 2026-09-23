# m5scp-timer

Timer with M5Stick CPlus

- [M5StickC Plus](https://docs.m5stack.com/ja/core/m5stickc_plus)

## Usage

When the device is power-on, it starts from the configuration state.

### Configuration state

- Button A
    - Push
        - When min/sec is selected: count up selected duration
        - Otherwise: start timer -> [Running state]
    - Hold
        - When min/sec is selected: count up
- Button B
    - Push
        - Select target duration: none -> min -> sec -> none -> min -> ...
    - Hold
        - When min/sec is selected: reset selected duration
        - Otherwise: reset all duration

### Running state

- Button A: stop timer -> [Stop state]

### Stop state

- Button A: restart timer -> [Running state]
- Button B: reset -> [Configuration state]

## License

Unlicense
