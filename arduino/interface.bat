@echo off
arduino-cli board list
REM ask user for port
set /p PORT=Enter port:
arduino-cli compile src/interface/interface.ino --fqbn arduino:avr:leonardo
arduino-cli upload src/interface/interface.ino --fqbn arduino:avr:leonardo --port %PORT%