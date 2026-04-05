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


can call function to save configuration settings

can call function to load configuration settings from file (ex. basic_confid.dat)

==================================================

waveform default: sine

frequency default: 440 Hz

amplitude default: 1

offset default: 0

output default: terminal

sample rate default: 48000 Hz

duration default: infinite
