RUN:
./sine_wave_generator_3 [waveform_type] [frequency] [amplitude] [offset] [arbitrary_file]

examples:

./sine_wave_generator_3 --> for all defaults

./sine_wave_generator_3 sine 880 1 0 --> for sine wave, 880 Hz, amplitude=1, offset=0

./sine_wave_generator_3 arb --> to load wave.txt

==================================================

waveform types: sine, square, tri, saw, arb

frequency range: 0.01 - 20000 Hz

amplitude range: 0.0 - 1.0

offset range: -1 - 1

output types: 0=DAC, 1=terminal, 2=audio, 3=file, 4=multi


can call function save_config_file to save configuration settings

can call function load_config_file to load configuration settings from file (ex. basic_config.dat)

==================================================

waveform default: sine

frequency default: 440 Hz

amplitude default: 1

offset default: 0

output default: terminal

sample rate default: 48000 Hz

duration default: infinite

=================================================

keyboard input guide (while program is running)

left/right keys: change waveform

up/down keys: increase/decrease frequency by 10%

+/- : increase/decrease amplitude by 0.05

[/]: increase/decrease offset by 0.05

OR 

type 'F'/'A'/'O' followed by desired frequency/amplitude/offset

type 'S' to save current settings to keyboard_settings.dat

type 'L' to load settings from keyboard_settings.dat

type 'Q' to quit
