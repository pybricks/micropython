// Board and hardware specific configuration
#define MICROPY_HW_BOARD_NAME                   "Raspberry Pi Build HAT"

// UART0 - to Pi
#define MICROPY_HW_UART0_TX  (0)
#define MICROPY_HW_UART0_RX  (1)
#define MICROPY_HW_UART0_CTS (2)
#define MICROPY_HW_UART0_RTS (3)

// I2C0 - ports 1,2 and accelerometer
#define MICROPY_HW_I2C0_SDA  (8)
#define MICROPY_HW_I2C0_SCL  (9)

// I2C1 - ports 3,4
#define MICROPY_HW_I2C1_SDA  (18)
#define MICROPY_HW_I2C1_SCL  (19)

#define MICROPY_HW_SPI_NO_DEFAULT_PINS (1)
#define MICROPY_HW_ENABLE_UART_REPL (1)

#define MICROPY_HW_ENABLE_USBDEV (0)
#define MICROPY_PY_OS_DUPTERM (0)

#define MICROPY_CONFIG_ROM_LEVEL (MICROPY_CONFIG_ROM_LEVEL_CORE_FEATURES)
#define MICROPY_PY_BUILTINS_MEMORYVIEW (1)
#define MICROPY_ENABLE_SCHEDULER (1)
#define MICROPY_HELPER_REPL (1)
#define MICROPY_KBD_EXCEPTION (1)
