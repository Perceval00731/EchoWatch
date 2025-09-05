# EchoWatch

This repository contains a PlatformIO project for developing an ESP32-based smartwatch interface.

## Structure

- `platformio.ini` – PlatformIO configuration.
- `src/` – Arduino source code. Generated UI files from SquareLine should be exported into `src/ui/`.
- `squareline/` – SquareLine Studio project files (design only).

## SquareLine workflow

1. Open the project inside `squareline/` with SquareLine Studio.
2. Export the UI using the PlatformIO template.
3. Copy the generated sources into `src/ui/` and build with PlatformIO.

## Tools

- Interface design: [SquareLine Studio](https://squareline.io/downloads)
- Build system: [PlatformIO](https://platformio.org/)
