# 5-Axis Robotic Arm Controller

A real-time control software for a 5-axis robotic arm developed on an **STM32** microcontroller.

The project uses **FreeRTOS**, message queues, hardware PWM and timer interrupts to receive motor commands and control five stepper motors independently.

## Overview

The controller is based on a simple task-based architecture:

```text
          Motor Commands
                 │
                 ▼
        ┌─────────────────┐
        │ Packet Parser   │
        │    Task         │
        └────────┬────────┘
                 │
          Message Queue
                 │
                 ▼
        ┌─────────────────┐
        │ Motor Control   │
        │     Task        │
        └────────┬────────┘
                 │
       ┌─────────┼─────────┐
       │         │         │
       ▼         ▼         ▼
    Motor 0   Motor 1   ... Motor 4
       │         │           │
       ▼         ▼           ▼
    Timer PWM Timer PWM   Timer PWM
```

The packet parsing and motor control operations run as separate RTOS tasks. Commands are passed between them using a FreeRTOS message queue.

## Main Features

* 5-axis stepper motor control
* STM32-based embedded system
* FreeRTOS task-based architecture
* CMSIS-RTOS API
* FreeRTOS message queues
* Hardware PWM for step generation
* Timer update interrupts for step counting
* Independent direction control for each motor
* Variable motor speed control
* Automatic motor stop when no command is received
* Reusable stepper motor driver structure

## RTOS Architecture

Two main tasks are used by the controller.

### Packet Parsing Task

The packet parsing task waits for incoming messages from the receive queue.

After receiving a packet, it checks the start and end markers:

```c
#define PACKET_START 0x3C3C3C3C
#define PACKET_END   0x3E3E3E3E
```

If the packet is valid, the five motor speeds are extracted and passed to the motor control task through another message queue.

```c
osMessageQueuePut(commandQueueHandle, &cmd_msg, 0, 0);
```

### Motor Control Task

The motor control task waits for a valid motor command.

For each motor, the sign of the speed determines the direction:

```c
if (speed[0] < 0) {
    stepper_set_direction(&my_stepper0, false);
    speed[0] = -speed[0];
}
else {
    stepper_set_direction(&my_stepper0, true);
}
```

The absolute speed value is then passed to the stepper driver.

```c
stepper_move_speed(&my_stepper0, speed[0]);
```

The same structure is used for all five motors.

## Stepper Driver

The stepper motors are represented using a common `stepper_t` structure.

Each motor keeps its own:

* Enable pin
* Direction pin
* Timer handle
* Remaining step count
* Moving state

This makes it possible to use the same driver functions for all five motors.

Example initialization:

```c
stepper_init(&my_stepper0, GPIOE, GPIO_PIN_2, GPIO_PIN_3, &htim2);
stepper_init(&my_stepper1, GPIOE, GPIO_PIN_4, GPIO_PIN_5, &htim3);
stepper_init(&my_stepper2, GPIOE, GPIO_PIN_1, GPIO_PIN_0, &htim4);
```

## Hardware PWM

The step signal is generated using the STM32 timer peripheral rather than manually toggling a GPIO in software.

The timer frequency is calculated according to the requested step speed:

```c
uint32_t target_freq = (uint32_t) steps_per_sec;
```

The timer prescaler and auto-reload value are then calculated and applied to the timer.

```c
__HAL_TIM_SET_PRESCALER(stepper->htim, psc);
__HAL_TIM_SET_AUTORELOAD(stepper->htim, arr);
```

A 50% duty cycle is used for the PWM signal:

```c
__HAL_TIM_SET_COMPARE(
    stepper->htim,
    TIM_CHANNEL_1,
    arr / 2
);
```

This approach moves the pulse generation to the timer hardware instead of relying on software delays.

## Step Counting

For movements with a fixed number of steps, the driver keeps track of the remaining steps.

The timer update interrupt is used to count the generated steps:

```c
if (stepper->remaining_steps > 0) {
    stepper->remaining_steps--;

    if (stepper->remaining_steps == 0) {
        stepper_stop(stepper);
    }
}
```

When the requested number of steps has been completed, the PWM output and timer interrupt are stopped.

## Safety / Timeout Behavior

The motor control task waits for new commands with a timeout.

If no command is received within the specified period, all five motors are stopped:

```c
stepper_stop(&my_stepper0);
stepper_stop(&my_stepper1);
stepper_stop(&my_stepper2);
stepper_stop(&my_stepper3);
stepper_stop(&my_stepper4);
```

This prevents the motors from continuing to run indefinitely when command communication is interrupted.

## Motor Control

The controller supports independent speed and direction control for all five axes.

```text
Command
   │
   ├── Motor 0 → Direction + Speed
   ├── Motor 1 → Direction + Speed
   ├── Motor 2 → Direction + Speed
   ├── Motor 3 → Direction + Speed
   └── Motor 4 → Direction + Speed
```

Negative speed values are interpreted as reverse direction, while positive values correspond to the opposite direction.

## Development Notes

The initial step generation approach used software GPIO toggling and delays.

The control logic was later moved toward a timer-based PWM implementation:

```text
Software GPIO Toggle
        │
        ▼
   HAL_Delay()
        │
        ▼
   CPU-dependent
   pulse generation

             ↓

Hardware PWM
        │
        ▼
 STM32 Timer Peripheral
        │
        ▼
 Hardware-generated
   step pulses
```

Using the timer peripheral reduces the amount of CPU work required for continuous pulse generation and makes the motor-control loop more suitable for a real-time RTOS environment.

## Technologies

* **MCU:** STM32
* **Language:** C
* **RTOS:** FreeRTOS
* **RTOS API:** CMSIS-RTOS
* **HAL:** STM32 HAL
* **Motor:** Stepper Motors
* **Pulse Generation:** Hardware PWM
* **Communication:** Message Queue based command processing
* **Timers:** STM32 Hardware Timers

## Project Structure

The main control logic is divided into:

```text
freertos.c
    ├── Packet parsing task
    ├── Motor control task
    ├── Message queues
    └── Stepper initialization

stepper.c
    ├── Stepper initialization
    ├── Enable / disable
    ├── Direction control
    ├── Speed calculation
    ├── Hardware PWM control
    └── Step counting
```

## My Contribution

I developed the embedded control software for the robotic arm, including the stepper motor driver, FreeRTOS task structure, message queues, motor speed/direction control and timer-based PWM generation.

The main focus was to build a control structure that could handle five motors while keeping command processing and motor control separated through RTOS tasks.

