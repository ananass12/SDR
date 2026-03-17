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

vector<complex<float>> barker_code(vector<complex<float>> IQ_bpsk){
    vector<complex<float>> IQ_barker = {
        {1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f},
        {-1.0f, 0.0f}, {-1.0f, 0.0f}, {1.0f, 0.0f}, {-1.0f, 0.0f}
    };

    IQ_barker.insert(IQ_barker.end(), IQ_bpsk.begin(), IQ_bpsk.end());
    
    return IQ_barker;
}

vector<complex<float>> upsampling(vector<complex<float>>IQ_bpsk, int sample_per_symbol){
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

// vector<float> srrc(int syms, float beta, int P, float t_off) {
//     // s = (4*beta/sqrt(P)) scales SRRC
//     // syms  = Half of Total Number of Symbols
//     int length_SRRC = P * syms * 2;
//     float start = (-length_SRRC / 2.0) + 1e-8 + t_off;
//     float stop = (length_SRRC / 2.0) + 1e-8 + t_off;
//     float step = 1.0;

//     vector<float> k;
//     for (float val = start; val <= stop + 1; val += step) {
//         k.push_back(val);
//     }

//     if (beta == 0) {
//         beta = 1e-8;
//     }

//     vector<float> s;
//     s.reserve(k.size());

//     const float pi = M_PI;

//     for (auto val : k) {
//         float denom = pi * (1 - 16 * pow(beta * val / P, 2));
//         float numerator = cos((1 + beta) * pi * val / P) + 
//                            (sin((1 - beta) * pi * val / P) / (4 * beta * val / P));
//         s.push_back((4 * beta / sqrt(P)) * numerator / denom);
//     }

//     return s;
// }


vector<float> srrc(int syms, float beta, int L, float fs) {
    // syms: полу-длина фильтра в символах
    // L: отсчётов на символ (samples_per_symbol)
    // beta: коэффициент скругления (0...1)
    
    int span = 2 * syms * L + 1;
    vector<float> h(span);
    
    
    for (int i = 0; i < span; i++) {
        // Нормированное время: t в единицах периода символа
        float t = (float)(i - syms * L) / (float)L;
        
        float result;

        if (beta == 0) {
            beta = 1e-8;
        }
        
        //  Случай t = 0 
        if (fabsf(t) < EPS) {
            result = 1.0f + beta * (4.0f / M_PI - 1.0f);
        }
        // beta = 0 (обычный sinc)
        else if (beta < EPS) {
            float pi_t = PI * t;
            result = sinf(pi_t) / pi_t;
        }
        else {
            float four_beta_t = 4.0f * beta * t;  
            float pi_t = PI * t;
            
            if (fabsf(fabsf(four_beta_t) - 1.0f) < EPS) {
                float arg = PI * (1.0f + beta) / (4.0f * beta);
                float val = (beta / sqrtf(2.0f)) * 
                            ((1.0f + 2.0f/M_PI) * sinf(arg) + 
                             (1.0f - 2.0f/M_PI) * cosf(arg));
                result = (t > 0) ? val : -val;
            }
            else {
                float num = sinf(pi_t * (1.0f - beta)) + 
                            four_beta_t * cosf(pi_t * (1.0f + beta));
                float denom = pi_t * (1.0f - four_beta_t * four_beta_t);
                result = num / denom;
            }
        }
        
        h[i] = isfinite(result) ? result : 0.0f;
    }
    
    float energy = 0.0f;
    for (float v : h) energy += v * v;
    if (energy > 1e-10f) {
        float scale = 1.0f / sqrtf(energy);
        for (float& v : h) v *= scale;
    }
    
    return h;
}

vector< complex<float> > convolve2(vector<complex<float>> &upsampled, vector<float> &b) {
    vector <complex<float>> convolved;
    for (int n = 0; n < (int)upsampled.size(); ++n) 
        {
            complex<float> sum(0.0f, 0.0f);
            for (int k = 0; k < (int) b.size(); ++k) 
            {
                int idx = n - k;
                if (idx >= 0 && idx < (int) upsampled.size()) { 
                sum += upsampled[idx] * b[k];
            }
        }
        convolved.push_back(sum);
        }
    return convolved;
}


vector<complex<float>> matched_filter(vector<complex<float>> input, int L, float beta) {
    // Генерация SRRC фильтра
    // beta = 0.75
    // syms = половина длины фильтра в символах (обычно 5-10)
    int syms = 5;  
    vector<float> srrc_filter_float = srrc(syms, 0.75, 16, 0);
    
    // Конвертация в float и нормализация
    vector<float> mask(srrc_filter_float.size());
    float sum = 0.0f;
    for (size_t i = 0; i < srrc_filter_float.size(); i++) {
        mask[i] = static_cast<float>(srrc_filter_float[i]);
        sum += mask[i];
    }
    
    // Нормализация фильтра (чтобы сумма коэффициентов = 1)
    if (sum != 0.0f) {
        for (size_t i = 0; i < mask.size(); i++) {
            mask[i] /= sum;
        }
    }
    
    int filter_len = mask.size();
    vector<complex<float>> IQ_matched(input.size());
    
    for (int i = 0; i < (int)input.size(); i++) {
        float sum_I = 0.0f;
        float sum_Q = 0.0f;
        
        for (int m = 0; m < filter_len; m++) {
            // Центрируем фильтр относительно текущей точки
            int idx = i - filter_len/2 + m;
            
            if (idx >= 0 && idx < (int)input.size()) {
                sum_I += input[idx].real() * mask[m];
                sum_Q += input[idx].imag() * mask[m];
            }
        }
        
        IQ_matched[i] = {sum_I, sum_Q};
    }
    
    return IQ_matched;
}

vector<complex<float>> clock_recovery_mueller_muller(const vector<complex<float>>& samples, float sps) {
    float mu = 0.01f;
    vector<complex<float>> out(samples.size() + 10);
    vector<complex<float>> out_rail(samples.size() + 10);
    size_t i_in = 0;
    size_t i_out = 2;
    while (i_out < samples.size() && i_in + 16 < samples.size()) {
        out[i_out] = samples[i_in];

        out_rail[i_out] = complex<float>(
            real(out[i_out]) > 0 ? 1.0f : 0.0f,
            imag(out[i_out]) > 0 ? 1.0f : 0.0f
        );

        complex<float> x = (out_rail[i_out] - out_rail[i_out - 2]) * conj(out[i_out - 1]);
        complex<float> y = (out[i_out] - out[i_out - 2]) * conj(out_rail[i_out - 1]);
        float mm_val = real(y - x);

        mu += sps + 0.01f * mm_val;
        i_in += static_cast<size_t>(floor(mu));
        mu = mu - floor(mu);
        i_out += 1;
    }
    return vector<complex<float>>(out.begin() + 2, out.begin() + i_out);
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

vector<complex<float>> downsampling(vector <float> erof, vector<complex<float>> IQ){
    vector <complex<float>> IQ_downsampling;

    for (int i = 0; i < (int)erof.size(); i++){
        int index = erof[i];
        if (index >= 0 && index < (int)IQ.size()) {
            IQ_downsampling.push_back(IQ[index]);
        }
    }

    return IQ_downsampling;
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

void to_buff(vector<complex<float>>& IQ_tx, int16_t* sdr_buff, size_t sample_per_symbol){
    for (size_t i = 0; i<sample_per_symbol; i++){
        sdr_buff[2*i] = (int16_t)(IQ_tx[i].real() * 20000.0f);
        sdr_buff[2*i + 1] = (int16_t)(IQ_tx[i].imag() * 20000.0f);
    }
}

void from_buff(vector<complex<float>>& IQ_rx, const int16_t* sdr_buff, size_t sample_per_symbol){
    IQ_rx.resize(sample_per_symbol);
    for (size_t i = 0; i<sample_per_symbol; i++){
        IQ_rx[i] = {
            (float)sdr_buff[2*i] / 32768.0f,
            (float)sdr_buff[2*i + 1] / 32768.0f
        };
    }
}

vector<float> arange(float start, float stop, float step) {
    vector<float> result;
    for (float val = start; val < stop; val += step) {
        result.push_back(val);
    }
    return result;
}

// Грубая оценка частотного оффсета 
float coarse_max_freq_calculation(const vector<complex<float>>& samples, 
                                  int sample_rate) {
    int buffer_size = samples.size();
    
    vector<complex<float>> matched_squared(buffer_size);
    vector<complex<float>> fft_out(buffer_size);
    vector<float> fft_abs(buffer_size);
    
    // Возведение в квадрат (убираем BPSK модуляцию)
    for (int i = 0; i < buffer_size; i++) {
        matched_squared[i] = samples[i] * samples[i];  // z²
    }
    
    // БПФ
    fftwf_plan p = fftwf_plan_dft_1d(buffer_size,
                                   reinterpret_cast<fftwf_complex*>(matched_squared.data()),
                                   reinterpret_cast<fftwf_complex*>(fft_out.data()),
                                   FFTW_FORWARD, 
                                   FFTW_ESTIMATE);
    fftwf_execute(p);
    fftwf_destroy_plan(p);  
    
    // Амплитудный спектр
    for (int i = 0; i < buffer_size; i++) {
        fft_abs[i] = abs(fft_out[i]);
    }
    
    // fftshift (центр спектра)
    vector<float> fft_abs_shifted(buffer_size);
    int half = buffer_size / 2;
    for (int i = 0; i < buffer_size; i++) {
        int src_idx = (i + half) % buffer_size;
        fft_abs_shifted[i] = fft_abs[src_idx];
    }
    
    // Частотная ось
    vector<float> freqs(buffer_size);
    float df = (float)sample_rate / buffer_size;
    for (int i = 0; i < buffer_size; i++) {
        freqs[i] = (-(float)sample_rate / 2.0f) + i * df;
    }
    
    // Поиск пика
    auto max_it = max_element(fft_abs_shifted.begin(), fft_abs_shifted.end());
    int index = distance(fft_abs_shifted.begin(), max_it);
    
    // Делим на 2 (потому что возводили в квадрат)
    return freqs[index] / 2.0f;
}

// Компенсация частотного оффсета
vector<complex<float>> coarse_freq_sync(const vector<complex<float>>& samples, float coarse_freq, int sample_rate) {
    int buffer_size = samples.size();
    vector<complex<float>> out(buffer_size);
    
    float Ts = 1.0f / (float)sample_rate;
    const float PI = 3.14159265358979f;
    
    for (int i = 0; i < buffer_size; i++) {
        float t = i * Ts;
        // Поворот в противоположном направлении
        complex<float> rot(0.0f, -2.0f * PI * coarse_freq * t);
        out[i] = samples[i] * exp(rot);
    }
    
    return out;
}

vector<complex<float>> freq_sync_bpsk(const vector<complex<float>>& samples){
    int N = static_cast<int>(samples.size());
    float phase = 0.0f;
    float freq = 0.0f;
    const float alpha = 8.0f;
    const float beta = 0.02f;
    vector<complex<float>> out;
    out.resize(N);

    for (int i = 0; i < N; ++i) {
        complex<float> exp_term = complex<float>(0.0, -phase);
        out[i] = samples[i] * exp(exp_term);

        float error = out[i].real() * out[i].imag();
        freq += (beta * error);
        phase += freq + (alpha * error);

        while (phase >= 2 * PI){phase -= 2 * PI;} 
        while (phase < 0.0f){ phase += 2 * PI; }
    }
    return out;
}

vector<complex<float>> cross_correlation(const vector<complex<float>>& signal, const vector<complex<float>>& barker) {
    int n = signal.size();
    int m = barker.size();
    
    if (n < m) return {};

    int result_size = n - m + 1;
    vector<complex<float>> result(result_size);

    vector<complex<float>> barker_conj(m);
    for(int i=0; i<m; ++i) {
        barker_conj[i] = conj(barker[i]);
    }

    for (int k = 0; k < result_size; ++k) {
        complex<float> sum(0.0f, 0.0f);
        for (int i = 0; i < m; ++i) {
            sum += signal[k + i] * barker_conj[i];
        }
        result[k] = sum;
    }

    return result;
}

int find_barker(const vector<complex<float>>& signal, const vector<complex<float>>& barker, float threshold_ratio) {
    if (signal.size() < barker.size()) return -1;
    
    vector<complex<float>> corr = cross_correlation(signal, barker);  
    
    int barker_idx = -1;
    
    for (int i = 0; i < (int)corr.size(); i++) {
        if (corr[i].real() >= threshold_ratio){
            barker_idx = i;
            break;
        }
    }
    
    // if (barker_idx >= 0) {
    //     if (max_val < threshold_ratio) {
    //         printf("Пик корреляции слишком слабый: %.3f (порог: %.3f)\n", max_val, threshold_ratio);
    //         return -1;
    //     }
    // }
    
    return barker_idx;  
}