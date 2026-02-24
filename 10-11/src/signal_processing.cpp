#include "signal_processing.h"
#include <iostream>

vector<complex<float>> to_bpsk(vector<int>bits){
    vector<complex<float>>  IQ_bpsk (bits.size());

    for (int i = 0; i < (int)bits.size(); i++){
        if (bits[i] == 1){
            IQ_bpsk[i] = complex<float>(1.0f, 0.0f); 
        }
        else {
            IQ_bpsk[i] = complex<float>(-1.0f, 0.0f); 
        }
    }
    return IQ_bpsk;

}

vector<complex<float>> upsampling(vector<complex<float>>IQ_bpsk, int num_bits, int sample_per_symbol){
    vector<complex<float>> IQ_upsampled ;
    IQ_upsampled.resize(IQ_bpsk.size() * sample_per_symbol);
    int index = 0;
    for (int i = 0; i < (int)IQ_bpsk.size(); i++){
        index = i* sample_per_symbol;
        IQ_upsampled[index] = IQ_bpsk[i];
        
        // заполнение нулями
        for (int sample = 1; sample < sample_per_symbol; sample++){
            IQ_upsampled[index + sample] = complex<float>(0.0f, 0.0f); 
        }
    }
    return IQ_upsampled;
}

vector<complex<float>> convolve(vector<complex<float>>IQ_upsampled, int sample_per_symbol){
    vector<complex<float>> IQ_convolved;
    vector <float> mask(sample_per_symbol, 1.0f);

    for (int i = 0; i < (int)IQ_upsampled.size(); i++) {
            float sum_I = 0.0;
            float sum_Q = 0.0;
            for (int m = 0; m < (int)mask.size(); m++) {
                int idx =  i - m;
                if (idx >= 0 && idx < (int)IQ_upsampled.size()){
                    sum_I += IQ_upsampled[idx].real() * mask[m];
                    sum_Q += IQ_upsampled[idx].imag() * mask[m];
                }
            }
            IQ_convolved.push_back(complex<float>(sum_I, sum_Q));
    }
    return IQ_convolved;
}

vector<float> symbol_sync(vector<complex<float>> IQ_convolved2, int samples_per_symbol){
    int K1, K2, p1, p2 = 0;
    float BnTs = 0.0001;
    int Nsps = samples_per_symbol;
    float Kp = 0.0002;
    float zeta = sqrt(2) / 2;
    float theta = (BnTs / Nsps) / (zeta + (0.25 / zeta));
    K1 = -4 * zeta * theta / ((1 + 2 * zeta * theta + pow(theta,2)) * Kp);
    K2 = -4 * pow(theta,2) / ((1 + 2 * zeta * theta + pow(theta,2)) * Kp);
    int tau = 0;
    float err;
    vector<float> erof;

    // Основной цикл
    for (int i = 0; i < (int)IQ_convolved2.size(); i += Nsps){
        err = (IQ_convolved2[i + Nsps + tau].real() - IQ_convolved2[i + tau].real()) * IQ_convolved2[i + (Nsps / 2) + tau].real() + (IQ_convolved2[i + Nsps + tau].imag() - IQ_convolved2[i + tau].imag()) * IQ_convolved2[i + (Nsps / 2) + tau].imag();
        
        // Вычисление пропорциональной и интегральной составляющих
        p1 = err * K1;
        p2 = p2 + p1 + err * K2;

        // Нормализация p2 в пределах [-1, 1]
        if (p2 > 1){
            p2 = p2 - 1;
        } 
        if (p2 < -1){
            p2 = p2 + 1;
        } 

        // Обновление значения смещения tau
        tau = ceil(p2 * Nsps);
        // Сохранение результатов текущей итерации
        erof.push_back(i + Nsps + tau);
    }
 
    return erof;
}

vector<complex<float>> downsampling(vector <float> erof, vector<complex<float>> IQ_convolved2){
    vector <complex<float>> IQ_true;

    for (int i = 0; i < (int)erof.size(); i++){
        int index = erof[i];
        if (index >= 0 && index < (int)IQ_convolved2.size()) {
            IQ_true.push_back(IQ_convolved2[index]);
        }
    }

    return IQ_true;
}

vector<int> from_bpsk(const vector<complex<float>>& IQ_true) {
    vector<int> bits;
    
    if (IQ_true.empty()) {
        return bits;
    }
    
    bits.resize(IQ_true.size());
    
    for (size_t i = 0; i < IQ_true.size(); i++) {
        if (IQ_true[i].real() > 0.0f) {
            bits[i] = 1;
        } else {
            bits[i] = 0;
        }
    }
    
    return bits;
}