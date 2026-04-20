# MA4830 Waveform Generator - QNX Build
## Null Terminators

### Compile
```
make
```

### Run
```
./wavegen                              (default: sine, 440Hz)
./wavegen square 200 0.8 0.1           (square wave, 200Hz, amp 0.8, offset 0.1)
./wavegen arb 100 1.0 0.0 wave1.txt   (arbitrary from file)
```

### Controls
```
Arrow Up/Down     Frequency (+/- 10%)
Arrow Left/Right  Cycle waveform type
+/-               Amplitude (+/- 0.05)
[/]               Offset (+/- 0.05)
1-5               Direct waveform select (1=Sine 2=Square 3=Tri 4=Saw 5=Arb)
w                 Cycle arbitrary wave file (wave.txt -> wave1.txt -> wave2.txt)
f                 Type frequency value
a                 Type amplitude value
o                 Type offset value
s                 Save settings to settings.dat
l                 Load settings from settings.dat
q                 Quit
Ctrl+C            Quit (SIGINT)
```

### Expected Output
- Oscilloscope: waveform on DAC channel 0
- Terminal: ASCII dashboard with live wave preview, frequency, amplitude, offset
- Potentiometer 0: controls amplitude (0.0-1.0)

### Files Required
```
main.c                    - Threading, mutex, keyboard loop (Walter)
hw.c / hw.h               - PCI DAC/ADC hardware layer (Qihong)
sine_wave_generator_3.c   - Waveform math (Trudy/Misha)
sine_wave.h               - Waveform header
ui_graphics.c / .h        - ASCII display (Jaz)
setup_input.c / .h        - Config parse, keyboard input (Alicia)
wave.txt / wave1.txt / wave2.txt / wave3.txt  - Arbitrary waveform data
```

### Left TODO
- Optional:Trigger / wait for signal (novelty)
- Optional: Get schematics or else we need to handdraw it