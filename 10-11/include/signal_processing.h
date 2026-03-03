#ifndef SIGNAL_PROCESSING_H
#define SIGNAL_PROCESSING_H

#include <vector>
#include <complex>
#include <cmath>

using namespace std;

vector<complex<float>> to_bpsk(vector<int> bits);
vector<complex<float>> upsampling(vector<complex<float>> IQ_bpsk, int num_bits, int sample_per_symbol);
vector<complex<float>> convolve(vector<complex<float>> IQ_upsampled, int sample_per_symbol);
vector<float> symbol_sync(vector<complex<float>> IQ_convolved2, int samples_per_symbol);
vector<complex<float>> downsampling(vector <float> erof, vector<complex<float>> IQ_convolved2);
vector<int> from_bpsk(const vector<complex<float>>& IQ_true);
void to_buff(vector<complex<float>>& IQ_data, int16_t* tx_buff, size_t sample_per_symbol);
void from_buff(vector<complex<float>>& IQ_rx, const int16_t* sdr_buff, size_t sample_per_symbol);

#endif 