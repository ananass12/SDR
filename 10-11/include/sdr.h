#ifndef SDR_H
#define SDR_H

#include <SoapySDR/Device.h>
#include <SoapySDR/Formats.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <complex.h>
#include <math.h>
#include <cmath>
#include <vector>
#include <time.h>
#include <cstring>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstdlib> 
#include <string.h>

using namespace std;

typedef struct {
    float i;
    float q;
} iq_sample_t;

typedef struct {
    SoapySDRDevice *device;
    SoapySDRStream *rxStream;
    SoapySDRStream *txStream;
    size_t rx_mtu;
    size_t tx_mtu;
    int sample_rate;
    int carrier_freq;
} sdr_device_t;

sdr_device_t* sdr_init(int use_usb);
int sdr_configure(sdr_device_t *sdr);
int sdr_read_samples(sdr_device_t *sdr, int16_t *rx_buffer, long long *timeNs);
int sdr_write_samples(sdr_device_t *sdr, int16_t *tx_buff, long long tx_time);
void sdr_cleanup(sdr_device_t *sdr);

#endif