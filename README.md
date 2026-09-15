# Model Satellite Flight Control System

Embedded flight control and telemetry software developed for a model satellite competition using an **STM32F4** microcontroller.

The project was developed around a hierarchical state machine to manage the different phases of the mission. The system handles flight-state transitions, altitude-based decisions, landing control, telemetry, recovery operations, and persistent mission state.

## Overview

The satellite software is organized around the different stages of the mission:

```text
Ready to Flight
       │
       ▼
    Rising
       │
       ▼
    Landing
       │
       ▼
    Leaving
       │
       ▼
 Payload Landing 1
       │
       ▼
    Hanging
       │
       ▼
 Payload Landing 2
       │
       ▼
    Recovery
       │
       ▼
Telemetry Deactivated
```

The state machine is implemented using **QP/C**, allowing the system to handle events and transitions without relying on a large blocking control loop.

## Main Features

* STM32F4-based embedded control system
* Hierarchical state machine using QP/C
* Event-driven state transitions
* Mission state persistence using RTC backup registers
* Periodic altitude calculation
* Landing control with PID update functions
* Telemetry packet generation
* SD card data logging
* Telemetry transmission
* Ground command processing
* Carrier separation control
* Parachute opening command
* Recovery buzzer control
* Timed operations using QP/C time events

## State Machine

The main flight states are implemented as separate state handlers.

Each state is responsible for the actions and events relevant to that part of the mission.

For example, during the rising phase, the system periodically checks the altitude status:

```c
BSP_Height_Stat stat = BSP_calculate_height();

if (stat == LANDING) {
    return Q_TRAN(&landing);
}
```

The landing state then continues the landing control and checks for the next mission phase.

This structure makes the flight logic easier to follow compared to putting the complete mission sequence inside a single loop.

## Hierarchical State Machine

Telemetry-related functionality is handled through a super-state:

```text
                 telemetryActive
                       │
       ┌───────────────┼────────────────┐
       │               │                │
 Ready to Flight     Rising          Landing
       │               │                │
       └───────────────┴────────────────┘
                       │
                    Leaving
                       │
                  Payload States
                       │
                    Recovery
```

States that use the active telemetry functionality return to `telemetryActive` for common event handling.

This allows common operations such as telemetry transmission and command processing to be handled separately from flight-state-specific logic.

## Persistent Mission State

One of the important parts of the system is the use of the STM32 RTC backup register to store the current mission state.

For example:

```c
HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, (uint32_t) 2);
```

The saved value is checked during startup:

```c
uint8_t value = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0);
```

The software can then select the appropriate state instead of always starting from the beginning of the mission.

This was useful for keeping track of the mission phase across resets or power interruptions.

## Timed Events

Several operations are controlled using QP/C time events.

Examples include:

* Periodic altitude calculation
* Periodic telemetry transmission
* 10-second mission delays
* Recovery buzzer timing

For example, the recovery state starts a timed buzzer event and a 10-second timer:

```c
QTimeEvt_armX(&me->tenSecTimeEvt, 10000U, 0U);
QTimeEvt_armX(&me->buzzTimeEvt, 500U, 500U);
```

This keeps timing-related operations event-driven instead of using blocking delays.

## Telemetry

While telemetry is active, the system periodically:

1. Creates a telemetry packet
2. Saves the packet to the SD card
3. Sends the packet

```c
BSP_Create_Packet();
BSP_Save_to_SD();
BSP_Send_Packet();
```

Ground commands are also processed through the telemetry state. Depending on the received command, the system can perform actions such as carrier separation or parachute deployment.

## Landing Control

During the landing phases, altitude information is used to determine the next state while the landing controller is updated periodically.

```c
BSP_Height_Stat stat = BSP_calculate_height();

BSP_PID_Update_landing();
```

A separate control function is also used for the hanging phase:

```c
BSP_PID_Update_hanging();
```

## Competition Result

The project was developed as part of a model satellite competition.

* **CDR Score:** 89.2
* Reached the stage immediately before the final round

## Technologies

* **MCU:** STM32F4
* **Language:** C
* **Framework:** QP/C
* **Control:** PID
* **Storage:** SD Card
* **Persistent State:** STM32 RTC Backup Registers
* **Development Environment:** STM32CubeIDE / STM32 HAL

## My Contribution

I worked on the embedded flight-control software, including the state-machine structure, mission-state handling, timed events, flight-state transitions, and control/telemetry related software.

The main goal was to keep the flight logic separated into manageable states and to avoid blocking operations where an event-driven approach was more suitable.
