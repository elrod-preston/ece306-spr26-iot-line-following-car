# ECE306 SPR26 IOT Line Following Car

## Course Result

This code finished the course on the first try.

Built for NCSU ECE306 SPR26, this project runs an MSP430FR2355 IOT line-following car with LCD feedback, ESP-based Wi-Fi control, calibrated IR line detection, motor PWM control, DAC-assisted motor power stabilization, and a command-driven course coordinator.

## Image File Setup

Store all README images inside an `images/` folder at the same level as this README.

Use these renamed image files:

| Original Image | New README Image Name | Purpose |
|---|---|---|
| `IMG_2664.JPG` | `images/car_side_profile.jpg` | Side view showing the chassis height, stacked boards, wheel layout, caster height, and sensor housing position. |
| `IMG_2665.JPG` | `images/car_underside_drive_base.jpg` | Underside/front view showing the battery pack, rear drive wheels, front caster, chassis plate, wiring pass-through, and front sensor housing. |
| `IMG_2666.JPG` | `images/startup_01_battery_switch_prompt.jpg` | Startup step 1: battery voltage and switch prompt. |
| `IMG_2668.JPG` | `images/startup_02_dac_ramp.jpg` | Startup step 2: DAC ramp state. |
| `IMG_2669.JPG` | `images/startup_03_iot_ip_display.jpg` | Startup step 3: IOT setup with IP/port display. |
| `IMG_2670.JPG` | `images/startup_04_sensor_calibration.jpg` | Startup step 4: calibrated sensor setup. |
| `IMG_2671.JPG` | `images/startup_05_waiting_for_input.jpg` | Startup step 5: ready state waiting for app input. |
| `IMG_2653.PNG` | `images/iot_app_button_layout.png` | IOT app control layout showing the saved button structure. |

## Getting Started

This section explains the normal startup and control process for the robot.

### 1. Build the Car Hardware

The robot uses a flat chassis plate with two rear drive wheels, a front caster, a battery holder, a stacked MSP430 control board, an ESP-based IOT module, and a front-mounted sensor housing.

![Car side profile](images/car_side_profile.jpg)

The side profile shows the low chassis height, rear drive wheel position, front caster support, stacked control board, and raised IOT module.

![Car underside and drive base](images/car_underside_drive_base.jpg)

The underside/front view shows the battery pack, rear motor/wheel layout, caster wheel, wiring pass-through, and front sensor housing used to shield the line sensors from outside light.

### 2. Power On the Car

When the car first powers on, it displays battery voltage and waits for switch input before continuing.

![Startup battery and switch prompt](images/startup_01_battery_switch_prompt.jpg)

The battery should be charged before running the course. If the displayed voltage is near **4.5 V**, replace or recharge the batteries because 16 MHz operation is more sensitive to voltage drops.

### 3. Wait for DAC Ramp

After the startup prompt, the car enters the DAC ramp state.

![DAC ramp startup state](images/startup_02_dac_ramp.jpg)

The DAC ramp helps stabilize the motor power/reference behavior before the robot performs motor-heavy work.

### 4. Wait for IOT Setup

After the DAC ramp finishes, the ESP IOT module connects to Wi-Fi and starts the TCP server.

![IOT IP display](images/startup_03_iot_ip_display.jpg)

The LCD shows the IP/server information. In the example image, the car is using port `8888`.

### 5. Calibrate the Sensors

After IOT setup, the car enters sensor calibration mode.

![Sensor calibration screen](images/startup_04_sensor_calibration.jpg)

Use the switch prompts to capture the white and black sensor readings. These values are used by the line-following logic and by command conditions such as `BLACK`, `BLACK100`, `WHITE`, and `WHITE100`.

### 6. Wait for Input

After calibration is complete, the car enters the ready state.

![Waiting for input screen](images/startup_05_waiting_for_input.jpg)

At this point, the robot is ready to receive commands from the IOT app.

### 7. Connect the IOT App

Open the IOT app and enter the IP address and port shown on the car LCD.

![IOT app button layout](images/iot_app_button_layout.png)

Example connection:

```text
IP Address: 10.154.38.60
Port: 8888
```

After connecting, send the configured authorization password if required by the code. Once authorized, the saved buttons can send manual movement commands, reset commands, display commands, and autonomous command sequences.

## IOT App Button Setup

The app uses saved buttons to send command strings to the robot. Manual movement buttons use `M(left,right,~)` so the car continues moving until another command, condition, or stop command ends the motion.

| App Button | Command to Send | Purpose |
|---|---|---|
| `FORWARD` | `M(+30,+25,~)` | Drives forward continuously. |
| `REVERSE` | `M(-30,-25,~)` | Drives backward continuously. |
| `LEFT` | `M(-30,+25,~)` | Tank-turns left continuously. |
| `RIGHT` | `M(+30,-25,~)` | Tank-turns right continuously. |
| `STOP` | `STOP` | Stops the active command and stops both motors. |
| `CENTER` | `BLKFL` | Starts black-line-follow mode. |
| `RST` | `RST` | Resets the command/course state. |
| `DISPLAY` | `PR(Arrived 07)` | Displays the pad 7 arrival message. |
| `8` | `PR(Arrived 08),PA10000,PR(BL Start),M(+30,+25,2000),M(+35,+45,~),WHITE,PA10000,M(+32,+25,~),BLACK100,PR(Intercept),PA10000,PR(BL Turn),M(+35,-35,600),PA10000,PR(BL Travel),BLKFL10000,PR(BL Circle),PA10000,BLKFL~` | Runs the pad 8 autonomous black-line sequence. |

## IOT Command Structure

Commands are received from the authorized IOT app connection and are processed as comma- or semicolon-separated tokens.

### Print Commands

`PR(text)` prints text to the top LCD row.

Examples:

```text
PR(Arrived 07)
PR(Arrived 08)
PR(BL Start)
PR(Intercept)
PR(BL Turn)
PR(BL Travel)
PR(BL Circle)
PR(BL Exit)
PR(BL Stop)
```

### Pause Commands

`PA1000` pauses with motors off for the selected number of milliseconds.

Examples:

```text
PA1000
PA5000
PA10000
```

### Motor Commands

`M(left,right,ms)` runs explicit left and right motor percentages for a duration.

Examples:

```text
M(+30,+25,2000)
M(+35,-35,600)
M(-30,-25,1000)
```

`M(left,right,~)` holds the motor command until another command or condition ends it.

Examples:

```text
M(+30,+25,~)
M(-30,-25,~)
M(-30,+25,~)
M(+30,-25,~)
```

Motor command format:

```text
M(left_pwm,right_pwm,time)
```

Where:

- Positive PWM drives that side forward.
- Negative PWM drives that side reverse.
- `0` stops that side.
- A number in the time field runs the command for that many milliseconds.
- `~` holds the command until the next condition, stop, or replacement command.

### Simple Movement Commands

The command coordinator also supports simple movement commands.

Timed examples:

```text
F1000
B1000
L500
R500
```

Held examples:

```text
F~
B~
L~
R~
```

### Sensor Wait Commands

Sensor wait commands use the calibrated white and black readings.

```text
BLACK
BLACK100
WHITE
WHITE100
```

Examples:

```text
M(+35,+45,~),WHITE
M(+32,+25,~),BLACK100
```

`BLACK100` waits for a black threshold with an added sensitivity offset.

`WHITE100` waits for a white threshold with an added sensitivity offset.

### Black-Line-Follow Commands

Black-line-follow commands start the follow addon.

```text
BLKFL
BLKFL10000
BLKFL~
```

Behavior:

- `BLKFL` starts black-line-follow mode.
- `BLKFL10000` follows the black line for 10000 ms.
- `BLKFL~` follows the black line until another command ends it.

### Stop Commands

```text
STOP
S
```

These stop the current course command and stop the motors.

### Reset Commands

```text
RST
RESET
```

These reset the course display, timer, command buffer, and command state.

## Pad 8 Autonomous Sequence

The pad 8 button sends this full command string:

```text
PR(Arrived 08),PA10000,PR(BL Start),M(+30,+25,2000),M(+35,+45,~),WHITE,PA10000,M(+32,+25,~),BLACK100,PR(Intercept),PA10000,PR(BL Turn),M(+35,-35,600),PA10000,PR(BL Travel),BLKFL10000,PR(BL Circle),PA10000,BLKFL~
```

Sequence behavior:

1. `PR(Arrived 08)` displays arrival at pad 8.
2. `PA10000` waits 10 seconds for TA confirmation.
3. `PR(BL Start)` displays the black-line start state.
4. `M(+30,+25,2000)` drives forward for 2 seconds.
5. `M(+35,+45,~),WHITE` keeps driving until the sensors see white.
6. `PA10000` waits 10 seconds.
7. `M(+32,+25,~),BLACK100` drives until the black line is detected.
8. `PR(Intercept)` displays the intercept state.
9. `PA10000` waits 10 seconds.
10. `PR(BL Turn)` displays the turn state.
11. `M(+35,-35,600)` performs a timed tank turn.
12. `PA10000` waits 10 seconds.
13. `PR(BL Travel)` displays the travel state.
14. `BLKFL10000` follows the black line for 10 seconds.
15. `PR(BL Circle)` displays the circle state.
16. `PA10000` waits 10 seconds.
17. `BLKFL~` continues black-line-following until another command is sent.

## Feature Overview

- **Main startup sequence:** `main.c` starts in a safe, low-power programming-friendly state so the processor can be flashed without turning on the battery, then steps through core initialization, DAC ramp, IOT setup, sensor calibration, and command coordination.
- **Cooperative task loop:** The main loop gives CPU time to each active task in order instead of blocking permanently inside one subsystem.
- **16 MHz CPU speed:** The MSP430 is configured for a 16 MHz MCLK with an 8 MHz SMCLK so the robot has enough processing headroom for IOT, LCD, sensor, and motor work.
- **Battery voltage warning:** If the battery reads around **4.5 V**, replace or recharge it because 16 MHz operation is more sensitive to voltage dips than the default lower-speed clock setup.
- **Precise system timing:** `system_ticks_ms` provides a millisecond timebase used for debounce, display updates, command timing, IOT retry timing, line-follow timing, and non-blocking delays.
- **Responsive switch inputs:** SW1 and SW2 are handled through interrupts with debounce timing and short backlight blink feedback so button presses are visible and responsive.
- **Custom LCD SPI driver:** The LCD driver was built custom from online reference material because the original/reference LCD behavior was not reliable enough after the 16 MHz clock change.
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

## Hardware / Debug Notes

- The ESP IOT module uses UCA0, while USB debugging uses UCA1.
- The LCD is a four-row, ten-column DOGS104-A style display driven through the custom SPI implementation.
- The app must send the configured authorization password before movement commands are accepted.
- Wi-Fi slot 1 is intended for the school network, while Wi-Fi slot 2 is intended for a personal backup network.
- Red/green IOT LEDs pulse for TX/RX activity and show connection status during setup.
- The LCD/backlight status gives visible feedback during startup, authorization, switch presses, and command execution.
- The front sensor housing helps reduce external light interference during line detection.
- The rear drive wheels are controlled independently, allowing forward motion, reverse motion, and tank turns.
- The front caster provides passive support while the rear wheels provide all driven motion.
