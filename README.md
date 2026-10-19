# STM32 CAN-Based Automotive Instrument Cluster

A two-node automotive-style instrument cluster built on STM32F103C8T6 (Blue Pill) boards.
A **sensor node** reads speed, distance and environment data and broadcasts it over a
CAN bus (MCP2515). A **display node** receives the frames and renders a live dashboard
on a 1.3" ST7789 TFT, navigated with a rotary encoder.

## Features

- Two-node architecture communicating over CAN (125 kbps, standard 11-bit IDs)
- Analog-style **speedometer** with a smoothed needle (exponential smoothing, partial redraw, no flicker)
- **Parking-assist screen** driven by ultrasonic distance (car icon, distance, signal bars)
- **Temperature / humidity** page
- Combined **dashboard** page showing all values at once
- Page navigation with a **KY-040 rotary encoder** (hardware encoder mode on TIM4)
- Custom bare-metal-style drivers written from scratch: ST7789, MCP2515, HC-SR04, DHT11, KY-040
- Built-in 5x7 bitmap font with scaling and automatic word wrap

## System Overview

```
 SENSOR NODE (STM32F103)                      DISPLAY NODE (STM32F103)
 ┌──────────────────────┐                     ┌──────────────────────┐
 │ HC-SR04  (distance)  │                     │ ST7789 TFT (SPI1)    │
 │ DHT11    (temp/hum)  │                     │ KY-040 encoder (TIM4)│
 │ Potentiometer (ADC)  │                     │ MCP2515 (SPI2)       │
 │ MCP2515  (SPI1)      │                     └──────────▲───────────┘
 └──────────┬───────────┘                                │
            └────────────── CAN bus (CANH / CANL) ───────┘
```

## CAN Protocol

| CAN ID  | Payload                  | Description               |
|---------|--------------------------|---------------------------|
| `0x100` | `uint16` (big-endian)    | Speed (0–180 km/h)        |
| `0x101` | `uint16` (big-endian)    | Distance (cm)             |
| `0x102` | `int8` temp, `uint8` hum | Temperature / humidity    |

Frames are sent every 100 ms. DHT11 is sampled every 2 s and HC-SR04 every 100 ms.

## Hardware

- 2x STM32F103C8T6 "Blue Pill"
- 2x MCP2515 CAN modules (8 MHz crystal, TJA1050, 120 Ω termination enabled on both ends)
- 1.3" ST7789 240x240 SPI TFT (no CS pin)
- HC-SR04 ultrasonic sensor, DHT11, potentiometer, KY-040 rotary encoder

## Software

- STM32CubeMX + HAL
- Keil MDK-ARM / STM32CubeIDE
- Microsecond timing for HC-SR04 / DHT11 via the DWT cycle counter

## Engineering Notes / Lessons Learned

- **CAN speed vs. wiring:** 500 kbps was unreliable over breadboard jumper wires; 125 kbps is stable.
- **RX buffer overflow:** drawing to the TFT on every received frame was slow enough that the MCP2515
  dropped frames. Fixed by decoupling reception (fast loop) from rendering (throttled), and enabling
  RXB0 rollover to RXB1.
- **Atomic frame read:** use the MCP2515 "Read RX Buffer" command to read ID, DLC and data in a single
  SPI transaction and avoid torn reads.
- **SPI timeouts:** replaced `HAL_MAX_DELAY` with finite timeouts so a glitch cannot freeze the loop.
- **Pin conflicts:** a GPIO used for the HC-SR04 trigger clashed with the ADC channel; always check
  pin alternate functions in CubeMX.
- **HC-SR04 spacing:** measurements must be spaced (~60 ms+) or consecutive triggers corrupt each other.
- **Startup timing:** some ST7789 modules need a startup delay and a repeated init sequence.

## Project Structure

```
Sensor-Node/     # HC-SR04, DHT11, ADC potentiometer, MCP2515 TX
Display-Node/    # ST7789 UI, KY-040 encoder, MCP2515 RX
```

## Roadmap

- Interrupt-driven CAN reception (MCP2515 INT pin)
- Independent watchdog (IWDG) and CAN bus-off recovery
- Use all three MCP2515 TX buffers
- Settings page

video demo: https://www.youtube.com/shorts/9TjNHiKE10Y
