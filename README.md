# Apex Rover — DFM & Robotics Engineering Project
![Apex Rover](./rover_shot.png)

A custom 4WD robotic rover developed as a hands-on Design for Manufacturing (DFM), mechanical design, fabrication, electronics, and embedded robotics project.

## Project Overview

The Apex Rover is a 4WD skid-steer robotic vehicle built around a dual-deck 3D-printed chassis.

The rover has two operating modes:

- Wi-Fi manual control through an ESP32-hosted web interface
- Autonomous obstacle avoidance using infrared sensing

The main goal of this project was to experience the complete engineering process from CAD design to physical manufacturing, testing, and redesign.

## Engineering Objectives

This project was developed to gain practical experience in:

- Mechanical CAD design
- Design for Manufacturing (DFM)
- FDM 3D printing
- Engineering tolerances
- Fastener and joint design
- Mechanical assembly
- Electrical system integration
- ESP32 development
- Embedded C++
- Robotics control
- Physical prototype testing
- Engineering iteration

The primary objective was not simply to build a working rover, but to understand how a digital design behaves when converted into a physical manufactured prototype.

## Mechanical Design

The rover uses a two-level chassis architecture.

The lower chassis supports:

- Four TT gearbox motors
- Motor drive hardware
- Battery system
- High-current wiring

The upper deck supports:

- ESP32 controller
- Breadboard
- IR sensing hardware
- Control wiring

### Main Dimensions

| Parameter | Specification |
| --- | --- |
| Chassis footprint | 180 x 140 mm |
| Upper deck thickness | 3.5 mm |
| Deck separation | 40 mm |
| Fastener system | M3 |
| FDM infill | 20% gyroid |
| Primary materials | PLA / PETG |

## Design for Manufacturing

The chassis was designed specifically for FDM 3D printing.

Important DFM considerations included:

### Fastener Hole Tolerancing

M3 clearance holes were modeled at approximately 3.4 mm rather than the nominal 3.0 mm bolt diameter.

This provides clearance for the M3 fastener and accounts for the dimensional behavior of FDM printing.

### Structural Pillars

The lower chassis incorporates integrated 40 mm pillars supporting the upper deck.

This reduces the number of separate structural components and simplifies assembly.

### Cable Routing

A 60 x 40 mm cable pass-through opening was incorporated into the upper deck.

This allows motor and power wiring to pass between the two decks while keeping the wiring organized.

## V1 Prototype and Engineering Lessons

The physical prototype revealed several design considerations that were not fully apparent from CAD alone.

The most important observation was minor rotational movement at the pillar joints during high-torque skid turns.

This demonstrated the importance of positive mechanical retention in dynamically loaded joints.

The prototype also demonstrated the difference between clearance holes and threaded holes. A 3.4 mm M3 clearance hole allows a bolt to pass through, but it cannot provide a threaded connection by itself.

These observations were used to develop the V2 design.

## V1 to V2 Improvements

| V1 Observation | V2 Design Response |
| --- | --- |
| Pillar joint wobble | M3 heat-set brass inserts |
| Clearance holes cannot provide direct M3 threading | Dedicated self-tapping hole geometry |
| Nut accessibility | Recessed hex-nut pockets |
| Shared motor and logic power architecture | Dedicated 5 V logic regulator |

## Electronics

The rover is controlled by an ESP32.

Main components include:

- ESP32 NodeMCU
- L298N dual H-bridge motor driver
- Four TT gearbox motors
- IR obstacle detection system
- 6 x AA battery enclosure
- Breadboard
- M3 structural hardware

### ESP32 Motor Control

| ESP32 GPIO | Function |
| --- | --- |
| GPIO 26 | Left motor forward |
| GPIO 27 | Left motor reverse |
| GPIO 14 | Right motor forward |
| GPIO 12 | Right motor reverse |
| GPIO 33 | IR obstacle sensor input |

## Software

The firmware is written in C++ for the ESP32.

The rover hosts a local web interface through the ESP32 Wi-Fi access point.

Manual control uses HTTP endpoints for forward, backward, left, right, stop, automatic mode, and manual mode.

Autonomous operation uses a non-blocking state machine based on millis().

Major autonomous states include:

- DRIVING
- AVOID_STOPPING
- AVOID_REVERSING
- AVOID_PAUSING
- AVOID_TURNING
- PATROL_TURNING

## Manufacturing Workflow

The prototype followed this general engineering workflow:

1. Define requirements
2. Develop chassis architecture
3. Create CAD geometry
4. Apply DFM considerations
5. Manufacture the 3D-printed components
6. Assemble the mechanical system
7. Install electronics
8. Route and connect wiring
9. Upload and verify firmware
10. Test the physical prototype
11. Identify design limitations
12. Develop V2 improvements

## Repository Contents

- `Apex_Rover_DFM_Report_MB.pdf` - Complete DFM and technical report
- `apex_rover.ino` - ESP32 firmware
- `lower_chassis.step` - Lower chassis CAD model
- `lower_chassis.stl` - Lower chassis 3D-print file
- `top_deck.step` - Top deck CAD model
- `top_deck.stl` - Top deck 3D-print file
- `2D_technical_drawing.png` - Dimensioned technical drawing

## Lessons Learned

This project demonstrated that DFM is an iterative engineering process.

The workflow is not simply:

CAD -> Print -> Done

Instead, the physical prototype creates information that can be used to improve the next design:

Design -> Manufacture -> Test -> Learn -> Redesign

The project provided practical experience with manufacturing tolerances, mechanical joints, fastening strategies, assembly, cable management, electrical integration, and prototype evaluation.

## Project Status

V1 prototype: Complete and physically validated.

V2: Engineering improvements identified and documented.

The repository contains the V1 CAD models, manufacturing files, firmware, technical drawing, and DFM report.

## Author

Mickael Bah

Mechanical Engineering / Robotics
