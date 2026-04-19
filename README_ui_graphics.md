# C89 UI Preview

## Purpose

This file is a terminal-based "interesting graphics" demo for the MA4830 waveform generator project.

It is meant to support the visual side of the project by providing:
- a clean terminal dashboard
- a live ASCII waveform preview
- runtime status display
- activity indicator
- message and error panels

This is **not** the DAC engine itself. It is the screen/UI layer that can be connected to the team's main program later.

---

## Features

- C89-compatible code style
- ASCII preview for:
  - sine
  - square
  - triangle
  - sawtooth
- terminal dashboard showing:
  - waveform
  - frequency
  - amplitude
  - mean
  - DAC status
  - ADC control status
  - DIO exit readiness
  - running state
- animated activity spinner
- example message box
- example error box
- ANSI terminal screen refresh

---

## Files

- `ui_preview_c89.c`  
  Main C89-compatible demo source file.

---

## Compile

Using GCC:

```bash
gcc -std=c89 -pedantic -Wall -Wextra ui_preview_c89.c -lm -o ui_preview

## Run

```bash
./ui_preview