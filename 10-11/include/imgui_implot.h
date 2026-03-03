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
    const vector<complex<float>>& IQ_upsampled, 
    const vector<complex<float>>& IQ_convolved, 
    const vector<complex<float>>& IQ_convolved2,
    const vector<complex<float>>& IQ_true,
    const vector<int> bits2,
    const vector<float>& erof,
    int samples_per_symbol
);

#endif