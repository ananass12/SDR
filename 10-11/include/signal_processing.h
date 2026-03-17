#ifndef SIGNAL_PROCESSING_H
#define SIGNAL_PROCESSING_H

#include <vector>
#include <complex>
#include <cmath>
#include <fftw3.h>
#include <complex>
#include <cmath>
#include <algorithm>    
#include <iterator> 

using namespace std;

const float PI = 3.14159265358979323846;
const float EPS = 1e-6f;

vector<complex<float>> to_bpsk(vector<int> bits);
vector<complex<float>> barker_code(vector<complex<float>> IQ_bpsk);
vector<complex<float>> upsampling(vector<complex<float>> IQ_bpsk, int sample_per_symbol);
vector<complex<float>> convolve(vector<complex<float>> IQ_upsampled, int sample_per_symbol);
vector<complex<float>> matched_filter(vector<complex<float>> input, int L, float beta = 0.75);
vector< complex<float> > convolve2(vector<complex<float>> &upsampled, vector<float> &b);

vector<float> symbol_sync(vector<complex<float>> IQ_convolved2, int samples_per_symbol);
vector<complex<float>> clock_recovery_mueller_muller(const vector<complex<float>>& samples, float sps);
vector<complex<float>> downsampling(vector <float> erof, vector<complex<float>> IQ_convolved2);
vector<int> from_bpsk(const vector<complex<float>>& IQ_true);
void to_buff(vector<complex<float>>& IQ_data, int16_t* tx_buff, size_t sample_per_symbol);
void from_buff(vector<complex<float>>& IQ_rx, const int16_t* sdr_buff, size_t sample_per_symbol);
vector<float> srrc(int syms, float beta, int P, float t_off);

vector<complex<float>> freq_sync_bpsk(const vector<complex<float>>& samples);
int find_barker_position(const vector<complex<float>>& signal, const vector<complex<float>>& barker, float threshold = 0.7f);
vector<complex<float>> cross_correlation(const vector<complex<float>>& signal, const vector<complex<float>>& barker);

int find_barker(const vector<complex<float>>& signal, const vector<complex<float>>& barker, float threshold_ratio = 0.7f);

float coarse_max_freq_calculation(const vector<complex<float>>& samples,  int sample_rate);
vector<complex<float>> coarse_freq_sync(const vector<complex<float>>& samples, float coarse_freq, int sample_rate);
vector<float> arange(float start, float stop, float step);

#endif 