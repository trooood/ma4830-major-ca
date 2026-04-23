#include <stdio.h>
#include <string.h>
#include "hw.h"

#ifdef __QNX__
/* ---- REAL QNX HARDWARE CODE (PCI MAP) ---- */
#include <hw/inout.h>
#include <sys/mman.h>
#include <unistd.h>

int hw_open(Device *d) {
    struct pci_dev_info info;
    int i;
    memset(&info, 0, sizeof(info));
    if (pci_attach(0) < 0) return -1;
    
    info.VendorId = 0x1307; 
    info.DeviceId = 0x01;
    
    d->hdl = pci_attach_device(0, PCI_SHARE|PCI_INIT_ALL, 0, &info);
    
    /* Gain I/O permissions before register access */
    if (ThreadCtl(_NTO_TCTL_IO, 0) == -1) return -1;
    if (!d->hdl) return -1;
    
    for (i = 0; i < 5; i++) {
        d->iobase[i] = mmap_device_io(0x0f, PCI_IO_ADDR(info.CpuBaseAddress[i]));
    }
        
    /* DIO Initialization */
    out8(d->iobase[3] + 7, 0x90); 

    /* ADC Initialization (MISSING IN ORIGINAL CODE) */
    out16(d->iobase[1] + 0, 0x60c0); /* INTERRUPT: Clear */
    out16(d->iobase[1] + 4, 0x2081); /* TRIGGER: 10MHz, clear, Burst off, SW trig */
    out16(d->iobase[1] + 6, 0x007f); /* AUTOCAL: default */
    out16(d->iobase[2] + 2, 0);      /* Clear ADC FIFO */
    
    return 0; /* SUCCESS */
}

int hw_read_switch(Device *d) {
    /* Read from DIO_PORTA  */
    unsigned char val = in8(d->iobase[3] + 4);
    return (val & 0x0F); /* 0x0F to read 4 switches */
}

void hw_dac(Device *d, int chan, unsigned short val) {
    /* DA_CTLREG = iobase[1] + 8, DA_Data = iobase[4] + 0 */
    out16(d->iobase[1] + 8, (chan == 0) ? 0x0a23 : 0x0a43);
    out16(d->iobase[4] + 2, 0); /* Clear DAC FIFO */
    out16(d->iobase[4] + 0, val);
}

void hw_close(Device *d) {
    hw_dac(d, 0, 0x7FFF); /* Reset DAC0 to mid-range */
    hw_dac(d, 1, 0x7FFF); /* Reset DAC1 to mid-range */
    pci_detach_device(d->hdl);
}

unsigned short read_adc(Device *d, unsigned short channel) {
    unsigned short adc_val;
    unsigned short chan_config;
    
    /* Set MUXCHAN (iobase[1] + 2) */
    chan_config = 0x0D00 | ((channel & 0x0F) << 4) | (channel & 0x0F);
    out16(d->iobase[1] + 2, chan_config);
    
    /* Start ADC conversion */
    out16(d->iobase[2] + 0, 0);
    
    /* Poll MUXCHAN status bit 14 */
    while(!(in16(d->iobase[1] + 2) & 0x4000));
    
    /* Read AD_DATA */
    adc_val = in16(d->iobase[2] + 0);
    
    return adc_val;
}

void reset_dac(Device *d) {
    out16(d->iobase[1] + 8, 0x0a23);
    out16(d->iobase[4] + 2, 0);
    out16(d->iobase[4] + 0, 0x8FFF);
    
    out16(d->iobase[1] + 8, 0x0a43);
    out16(d->iobase[4] + 2, 0);
    out16(d->iobase[4] + 0, 0x8FFF);
}

#else
/* ---- MOCK HARDWARE CODE (Runs on Windows/Linux locally) ---- */

int hw_open(Device *d) {
    (void)d;
    printf("[MOCK] Hardware opened successfully. Bypassing PCI checks.\n");
    return 0;
}

int hw_read_switch(Device *d) { 
    (void)d; 
    return 1; 
}

void hw_dac(Device *d, int chan, unsigned short val) {
    (void)d; 
    (void)chan;
    (void)val;
}

void hw_close(Device *d) {
    (void)d;
    printf("\n[MOCK] Hardware closed.\n");
}

#endif