# MA4830 Waveform Generator - Final Build
## Null Terminators

NOTE THAT THE TEXT FILE IN THE MIDDLE OF THIS FOLDER IS FOR WINDOWS TESTING

### Compile
```
make clean
make
```

### Run
```
./wavegen                              (default: sine, 5Hz)
./wavegen square 3 0.8 0.1            (square wave, 3Hz, amp 0.8, offset 0.1)
./wavegen arb 2 1.0 0.0 data/wave1.txt   (arbitrary from file)
```

### Startup
Program launches with a welcome screen. Select waveform type (1-5) or press Enter for default (Sine).
Program starts PAUSED. Press 'p' to begin waveform output (trigger mode).

### Keyboard Controls
```
p                 Pause/Resume waveform output (trigger)
m                 Mute/Unmute audio beep

Arrow Up/Down     Frequency (+/- 10%)
Arrow Left/Right  Cycle waveform type
+/-               Amplitude (+/- 0.05)
[/]               Offset (+/- 0.05)

1-5               Direct waveform (1=Sine 2=Square 3=Tri 4=Saw 5=Arb)
w                 Cycle arbitrary wave file

f                 Type exact frequency (with input validation and retry)
a                 Type exact amplitude (with input validation and retry)
o                 Type exact offset (with input validation and retry)

s                 Save settings to settings.dat
l                 Load settings from settings.dat

q / Ctrl+C        Quit (graceful shutdown)
```

### Hardware Controls (QNX Lab)
```
SW1               Run/Stop (hardware graceful termination)
SW2 ON            Potentiometer 0 controls amplitude
SW2 OFF           Keyboard controls amplitude
SW3 ON            Potentiometer 1 controls frequency
SW3 OFF           Keyboard controls frequency
SW4               Toggle audio beep
```

### Hardware Limits
```
Frequency:  0.01 - 10.0 Hz  (defined in setup_input.h)
Amplitude:  0.0  - 1.0
Offset:    -1.0  - 1.0
DAC:        16-bit (0x0000 - 0xFFFF)
Samples:    100 per cycle
```

### Expected Output
- Oscilloscope (DAC Ch0): Live waveform matching selected type
- Terminal: ASCII dashboard showing waveform preview, parameters, status
- Status shows STOPPED when paused, RUNNING when active
- Message box shows "PAUSED - Press 'p' to start" or "Running"
- Error beep sounds on invalid manual input (f/a/o keys)
- Cycle beep when audio enabled (m key or SW4)

### Features Implemented
- 3-thread architecture (wave output, display, keyboard input)
- Mutex-protected shared state with dirty flag (Option B)
- SCHED_FIFO real-time priority for wave thread
- QNX hardware timer (timer_create/sigwaitinfo) with nanosleep fallback
- 5 waveform types: sine, square, triangle, sawtooth, arbitrary
- Arbitrary waveform from file with amplitude/offset scaling
- Multiple arbitrary wave files (w key cycles through data/*.txt)
- Non-blocking keyboard input (termios raw mode / Windows conio.h)
- Arrow key waveform cycling and frequency control
- Manual parameter entry with strtod validation and retry
- Potentiometer control via ADC (amplitude and frequency)
- DIO switch control (run/stop, ADC toggle, audio toggle)
- Save/load configuration to file
- Pause/trigger mode (starts paused, p to begin)
- Audio feedback (error beeps + cycle beep)
- SIGINT graceful shutdown with hardware reset
- Cross-platform: #ifdef __QNX__ / _WIN32 guards throughout
- TODO: File scanner for arbitrary waveforms (Alicia, placeholder in main.c)
- TODO: Welcome screen function (Misha, placeholder in main.c)

### Files
```
main.c                     - Threading, mutex, keyboard loop (Walter)
Makefile                   - Build configuration
src/hw.c                   - PCI DAC/ADC/DIO hardware layer (Qihong)
src/hw.h                   - Hardware header
src/sine_wave_generator_3.c - Waveform math (Trudy/Misha)
src/sine_wave.h            - Waveform header
src/ui_graphics.c          - ASCII display and wave preview (Jaz)
src/ui_graphics.h          - Display header
src/setup_input.c          - Config parse, keyboard, validation (Alicia)
src/setup_input.h          - Config header + hardware limit constants
data/wave.txt              - Arbitrary waveform data
data/wave1.txt             - Arbitrary waveform data
data/wave2.txt             - Arbitrary waveform data
data/wave3.txt             - Arbitrary waveform data
```

### Photos/video Needed for Report
1. SINE wave (1)
2. SQUARE wave (2)
3. TRIANGLE wave (3)
4. SAWTOOTH wave (4)
5. ARBITRARY waveform from file (5)
6. frequency change (before/after) (Arrow keys)
7. amplitude change via potentiometer (+/-)
8. offset change via potentiometer ([/])
9. Welcome screen with waveform selection
10. Dashboard in PAUSED state (Start and midway) (P)
11. Manual frequency input (f key) with error retry
12. Manual amplitude input (a key)
13. Manual offset input (o key)
14. Arbitrary waveform loaded (w key cycling)
15. Hardware setup (PCI card, switches, potentiometer)
16. Sound key per cycle (m)
17. all breadboard interatctions
18. Saving and loading (S/L)
19. graceful shutdown (ctrl-c and q)