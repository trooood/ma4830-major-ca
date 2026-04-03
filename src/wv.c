#include <math.h>
#include "wv.h"

unsigned short wv_get(Mode m, double phase, double amp) {
    double v = 0;
    switch(m) {
        case SINE: v = (sin(2 * PI * phase) + 1) / 2; break;
        case SQUARE: v = (phase < 0.5) ? 1 : 0; break;
        case TRI: v = (phase < 0.5) ? (phase * 2) : (2 - phase * 2); break;
        case SAW: v = phase; break;
    }
    return (unsigned short)(v * amp * 65535);
}