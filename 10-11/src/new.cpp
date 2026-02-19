// начала переписывать всё заново полностью под C++ с использованием структуры вектор
#include <iostream>
#include <stdio.h>
#include <vector>
#include <complex.h>
#include <stdio.h>
#include <cmath>
#include <string.h>
#include "imgui.h"
#include "implot.h"
#include "signal_processing.h" 

using namespace std;

int main(){

    const int num_bits = 10;
    const int samples_per_symbol = 10;
    vector<int> bits;
    bits.resize(num_bits);
    
    cout << "Оригинальные биты" <<endl;

    for (int i = 0; i < num_bits; i++){
        bits[i] = rand() % 2;
        cout << bits[i] << " ";
    }

    cout << endl;
    cout << "Комплексные значения" <<endl;

    vector<complex<float>> IQ_bpsk = to_bpsk(bits);

    for (int i = 0; i < num_bits; i++){
        cout << IQ_bpsk[i] << " ";
    }
    
    cout << endl;
    cout << "После апсемплинга" <<endl;

    vector<complex<float>> IQ_upsampled = upsampling(IQ_bpsk, num_bits, samples_per_symbol);

    for (int i = 0; i < (int)IQ_upsampled.size(); i++){
        cout << IQ_upsampled[i] << " ";
    }
    cout << endl;
    cout << "После свертки" <<endl;

    vector<complex<float>> IQ_convolved = convolve(IQ_upsampled, samples_per_symbol);

    for (int i = 0; i < (int)IQ_convolved.size(); i++){
        cout << IQ_convolved[i] << " ";
    }
    cout << endl;
    cout << "После повторной свертки" <<endl;

    vector<complex<float>> IQ_convolved2 = convolve(IQ_convolved, samples_per_symbol);

    for (int i = 0; i < (int)IQ_convolved2.size(); i++){
        cout << IQ_convolved2[i] << " ";
    }
    cout << endl;

    vector<float> erof = symbol_sync(IQ_convolved2, samples_per_symbol);

    cout << "Массив индексов отсчетов правильных символов" <<endl;
    for (int i = 0; i < (int)erof.size(); i++){
        cout << erof[i] << " ";
    }
    cout << endl;

}
