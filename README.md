# Hextone

A modular hexagonal midi controller.

## Folder Structure
- software/ - Software that runs on the computer plugged into the controller
- scad/ - OpenSCAD models for the controller
- arduino/ - Arduino project that contains 2 seperate sketches
- arduino/src/module/ - Software that runs on every module (arduino)
- arduino/src/interface/ - Software that runs on the "interface" module (arduino)

## Modules

There is 2 types of modules:
- Interface Module
- Module

The interface module doesn't have any keys, but has a USB port, magnetic connectors, and a reset button (if something goes wrong, this will reset everything (will disconnect power to the rest of the modules for a few seconds)).

The normal modules have keys, and magnetic connectors, they are useless without the interface module.
It can handle up to 1020 modules in one system (a system is a group of modules that are connected to a single interface module).
But that many would be way too much, so it might be limited to 127 instead.

As of right now, only the interface module is working, and it also contains the keys, this is a proof of concept version, but later it will be updated to be able to control the modules.

## Software

The software is written in C++ and uses a custom serial library made by me.