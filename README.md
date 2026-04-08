### COMPILE 
cc -o sine_wave_generator_3 sine_wave_generator_3.c -lm

### RUN
- ./sine_wave_generator_3 (for default sine wave)
- ./sine_wave_generator_3 square (square wave)
- ./sine_wave_generator_3 tri (triangular wave)
- ./sine_wave_generator_3 saw (sawtooth wave)
- ./sine_wave_generator_3 arb (arbitrary form)

current program only allows for the above waves and if we write something else like ./ ... abc, an error message would pop up (i don't think this is necessary if we're calling it from main)

I have done Function 1 and 2 of our assignment thoroughly but have not answered the sub questions in the MA4830PracticalHelp PDF

You can continue from Function 3 onwards (wait for keyboard input)

i will also focus on being able to specify which file to read the arb waveform from

the important value to be sent to the oscilloscope is **wave_buffer[i]**

### update about updating line
see example code CLK_signal.c
to prevent the output from flooding the terminal while the wave is generated
