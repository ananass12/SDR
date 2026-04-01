#ifndef IMGUI_IMPLOT_H
#define IMGUI_IMPLOT_H

#include <stdio.h>
#include <complex.h>
#include <math.h>
#include <vector>
#include <iostream>

#include "imgui.h"
#include "implot.h"
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"

using namespace std;

void run_gui(
    const vector <int>& bits, 
    const vector<complex<float>>& IQ_bpsk,
    const vector<complex<float>>& IQ_ofdm, 
    const vector<complex<float>>& IQ_ofdm_demodulate,
    const vector<complex<float>>& rx_subset,
    const vector<complex<float>>& IQ_convolved2,
    const vector<complex<float>>& IQ_true,
    const vector<complex<float>>& IQ_corr,
    const vector<complex<float>>& znach_corr,
    const vector<complex<float>>& data_only,
    const vector<int> bits2,
    int samples_per_symbol
);

#endif