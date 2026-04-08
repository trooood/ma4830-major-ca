/* ************************************************************************** */
/* hw.h - Hardware Abstraction Layer Header                                   */
/* ************************************************************************** */

#ifndef HW_H
#define HW_H

#include <hw/pci.h>
#include <sys/neutrino.h>

/* Function Prototypes */
void* setup_pci(struct pci_dev_info *info, uintptr_t *iobase);
void generate_sine_wave(unsigned int *buffer, int samples, float amplitude);
void output_to_oscilloscope(uintptr_t *iobase, unsigned int *buffer, int samples);
void reset_dac(uintptr_t *iobase);

unsigned short read_adc(uintptr_t *iobase, unsigned short channel);

#endif /* HW_H */