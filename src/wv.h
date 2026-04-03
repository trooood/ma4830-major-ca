#ifndef WV_H
#define WV_H
#define PI 3.14159265
typedef enum { SINE, SQUARE, TRI, SAW } Mode;
unsigned short wv_get(Mode m, double phase, double amp);
#endif