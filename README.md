# m5scp-timer

Timer with M5Stick CPlus

- [M5StickC Plus](https://docs.m5stack.com/ja/core/m5stickc_plus)

## Usage

When the device is power-on, it starts from the configuration mode.

### Configuration mode

- Button A
    - Push
        - When min/sec is selected, count up selected duration
        - Otherwise, start timer -> [Running mode]
    - Hold
        - When min/sec is selected, count up selected duration while holding the butotn
- Button B
    - Push
        - Select target duration
            - Rotate the target by push: min -> sec -> finish -> min -> ...
            - When finished, configuration is saved
    - Hold
        - When min/sec is selected, reset selected duration
        - Otherwise, reset all duration
            - Configuration is saved on reset

### Running mode

- Button A: stop timer -> [Stop mode]

### Stop mode

- Button A: restart timer -> [Running mode]
- Button B: reset -> [Configuration mode]

## License

Unlicense
