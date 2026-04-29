# ECE306 SPR26 IOT Line Following Car

## Course Result

This code finished the course on the first try.

Built for NCSU ECE306 SPR26, this project runs an MSP430FR2355 IOT line-following car with LCD feedback, ESP-based Wi-Fi control, calibrated IR line detection, motor PWM control, DAC-assisted motor power stabilization, and a command-driven course coordinator.

## Feature Overview

- **Main startup sequence:** `main.c` starts in a safe, low-power programming-friendly state so the processor can be flashed without turning on the battery, then steps through core initialization, DAC ramp, IOT setup, sensor calibration, and command coordination.
- **Cooperative task loop:** The main loop gives CPU time to each active task in order instead of blocking permanently inside one subsystem.
- **16 MHz CPU speed:** The MSP430 is configured for a 16 MHz MCLK with an 8 MHz SMCLK so the robot has enough processing headroom for IOT, LCD, sensor, and motor work.
- **Battery voltage warning:** If the battery reads around **4.5 V**, replace or recharge it because 16 MHz operation is more sensitive to voltage dips than the default lower-speed clock setup.
- **Precise system timing:** `system_ticks_ms` provides a millisecond timebase used for debounce, display updates, command timing, IOT retry timing, line-follow timing, and non-blocking delays.
- **Responsive switch inputs:** SW1 and SW2 are handled through interrupts with debounce timing and short backlight blink feedback so button presses are visible and responsive.
- **Custom LCD SPI driver:** The LCD driver was built custom from online reference material because the original/reference LCD behavior was not reliable enough after the 16 MHz overclock.
- **DAC power stabilization:** The DAC subsystem ramps and settles the motor power/reference behavior before the course starts so the drive system is more stable.
- **Motor driver:** The motor layer uses global requested state/speed values, then applies them through `Motor_Task()` to keep motor commands centralized and predictable.
- **Serial mirror debugging:** The serial system mirrors IOT module traffic to USB so ESP commands, responses, and app communication can be watched from a terminal.
- **IOT boot sequence:** The ESP setup state machine resets the module, sends the AT setup sequence, joins Wi-Fi, opens the TCP server, and falls back from Wi-Fi slot 1 to Wi-Fi slot 2 if needed.
- **IOT status indication:** LEDs and LCD/backlight behavior show connection, authorization, RX activity, and TX activity without needing a debugger.
- **Line following addon:** The follow addon uses calibrated left/right IR readings to drive forward, correct left/right, or search for the line.
- **Command addon:** The command coordinator parses app commands such as print, motor, pause, movement, black-line-follow, wait-for-black/white, stop, and reset commands.

## Main Program Flow

`main.c` runs a simple state machine:

1. **Init Core:** Initializes ports, clocks, ADC, timers, serial, LCD, display, interrupts, motors, and shows battery voltage.
2. **DAC Ramp:** Starts the DAC subsystem and waits for the DAC ramp to finish before motor-heavy work begins.
3. **IOT Setup:** Resets and configures the ESP module until Wi-Fi, TCP server, and app authorization are ready.
4. **Calibrate Sensor:** Uses switch input and detector readings to capture white and black sensor calibration values.
5. **Command Coordinator:** Runs IOT receive handling, detector updates, command parsing, motor tasks, and display updates.

## Command Structure

Commands are received from the authorized IOT app connection and are processed as comma- or semicolon-separated tokens.

- `PR(text)` prints text to the top LCD row.
- `M(left,right,ms)` runs explicit left/right motor percentages for a duration.
- `M(left,right,~)` holds a motor command until another command or condition ends it.
- `PA1000` pauses with motors off for 1000 ms.
- `F1000`, `B1000`, `L500`, and `R500` run simple timed movements.
- `F~`, `B~`, `L~`, and `R~` hold simple movement commands.
- `BLACK`, `BLACK100`, `WHITE`, and `WHITE100` wait for calibrated sensor conditions.
- `BLKFL`, `BLKFL1000`, and `BLKFL~` start black-line-follow behavior.
- `STOP` or `S` stops the current course.
- `RST` or `RESET` resets the course display, timer, command buffer, and state.

## Core Files

- `Core/core.h` includes the core project interfaces used by `main.c`.
- `Core/lib/adc.h` defines ADC channels, battery divider values, and ADC read prototypes.
- `Core/lib/dac.h` defines DAC ramp values, DAC globals, and DAC function prototypes.
- `Core/lib/display.h` exposes the buffered LCD display helper functions.
- `Core/lib/functions.h` keeps legacy project prototypes available for compatibility.
- `Core/lib/init.h` exposes the full-board initialization routine.
- `Core/lib/interupt.h` defines interrupt timing constants, switch flags, system ticks, and backlight helpers.
- `Core/lib/iot.h` defines ESP Wi-Fi credentials, IOT timing values, message buffers, status globals, and IOT APIs.
- `Core/lib/LCD.h` defines the low-level DOGS104-A LCD driver interface and legacy LCD symbols.
- `Core/lib/macros.h` stores shared timing, state, reset, and PWM macros.
- `Core/lib/motor.h` defines motor direction states, PWM tuning values, requested motor globals, and motor APIs.
- `Core/lib/ports.h` maps MSP430 port bits to board signals and declares port initialization functions.
- `Core/lib/serial.h` defines UCA0/UCA1 ring buffers, legacy IOT buffers, baud selection, and serial APIs.
- `Core/lib/timers.h` exposes TimerB0 and TimerB3 initialization.
- `Core/lib/utils.h` exposes simple string-formatting helpers for display output.
- `Core/src/adc.c` configures the ADC and provides blocking raw/millivolt channel reads.
- `Core/src/clocks.c` configures the 16 MHz clock system and DCO trim.
- `Core/src/dac.c` configures the SAC3 DAC and runs the DAC ramp logic.
- `Core/src/display.c` manages four fixed-width LCD display rows and refresh requests.
- `Core/src/init.c` calls the board initialization functions in the required startup order.
- `Core/src/interupt.c` implements switch interrupts, debounce, backlight pulse timing, display timing, detector request timing, DAC overflow handling, and `system_ticks_ms`.
- `Core/src/iot.c` implements the ESP AT-command setup state machine, Wi-Fi fallback, TCP server setup, app authorization, RX parsing, and IOT status feedback.
- `Core/src/LCD.c` implements the custom DOGS104-A SPI LCD driver and compatibility functions required by older display code.
- `Core/src/motor.c` applies requested motor globals to TimerB3 PWM outputs with minimum PWM and wheel-balance offset handling.
- `Core/src/ports.c` configures all GPIO, ADC pins, UART pins, SPI pins, motor PWM pins, switch interrupts, and IOT control pins.
- `Core/src/serial.c` implements UCA0/UCA1 serial drivers, ring buffers, TX interrupt handling, and USB mirroring of IOT traffic.
- `Core/src/system.c` contains system-level helper behavior such as enabling interrupts.
- `Core/src/timers.c` configures TimerB0 for system timing and TimerB3 for motor PWM.
- `Core/src/utils.c` implements simple number-to-string formatting for display-safe output.

## Addon Files

- `Addon/lib/calibrate.h` defines calibration states, calibration globals, and calibration APIs.
- `Addon/lib/command.h` exposes the command coordinator APIs used by the main command state.
- `Addon/lib/detect.h` exposes detector raw values, detector request flag, and detector APIs.
- `Addon/lib/follow.h` exposes the line follower initialization and task functions.
- `Addon/lib/led.h` exposes LED initialization and legacy LED state-machine hooks.
- `Addon/lib/line.h` exposes the line intercept/alignment sequence API.
- `Addon/lib/shape.h` exposes the inactive legacy shape-motion demo API.
- `Addon/src/calibrate.c` captures white and black sensor readings through a switch-driven calibration state machine.
- `Addon/src/command.c` parses and executes IOT app commands, manages course timing, waits on detector conditions, updates LCD status, and coordinates motors/follow behavior.
- `Addon/src/detect.c` samples the left and right IR detector ADC channels and stores averaged raw readings.
- `Addon/src/follow.c` uses calibrated detector values to run a constant-speed black-line follower.
- `Addon/src/led.c` implements LED strobe behavior using the system millisecond tick.
- `Addon/src/line.c` implements a robust non-PID line intercept and alignment sequence.
- `Addon/src/shape.c` contains an inactive legacy timed-motion script kept for reference.

## Hardware/Debug Notes

- The ESP IOT module uses UCA0, while USB debugging uses UCA1.
- The LCD is a four-row, ten-column DOGS104-A style display driven through the custom SPI implementation.
- The app must send the configured authorization password before movement commands are accepted.
- Wi-Fi slot 1 is intended for the school network, while Wi-Fi slot 2 is intended for a personal backup network.
- Red/green IOT LEDs pulse for TX/RX activity and show connection status during setup.
- The LCD/backlight status gives visible feedback during startup, authorization, switch presses, and command execution.
