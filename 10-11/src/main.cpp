
#include <SoapySDR/Device.h>   
#include <SoapySDR/Formats.h>  
#include <stdlib.h>            
#include <stdint.h>
#include <complex.h>
#include <stdio.h>
#include <math.h>
#include <cstdlib> 
#include <string.h>

using namespace std;

void to_bpsk(int bits[], int count, float I_bpsk[], float Q_bpsk[]){
        for (int i = 0; i < count; i++){
            if (bits[i] == 0){    
                I_bpsk[i] = 1.0;   
                Q_bpsk[i] = 0.0;
            }
            else  {
                I_bpsk[i] = -1.0;    
                Q_bpsk[i] = 0.0;
            }
            //printf("(%.1f,%.1f) ", I_bpsk[i], Q_bpsk[i]);
        }
        //printf("\n");
}     

void upsampling(int duration, int count, float I_bpsk[], float Q_bpsk[], float I_upsampled[], float Q_upsampled[]){
    int index = 0;
    for (int i = 0; i < count; i++){
        I_upsampled[index] = I_bpsk[i];
        Q_upsampled[index] = Q_bpsk[i];
        for (int sample = 1; sample < duration; sample++){
            I_upsampled[index + sample] = 0.0;
            Q_upsampled[index + sample] = 0.0;
        }
        index += duration;
    }
}

void svertka(int sample, float I_upsampled[], float Q_upsampled[], float I_filtred[], float Q_filtred[]){
    const int length = 10;
    float mask[length];
    for (int i = 0; i < length; i++) {
        mask[i] = 1.0f;
    }

    for (int n = 0; n < sample; n++) {
        float sum_I = 0.0f;
        float sum_Q = 0.0f;
        
        for (int k = 0; k < length; k++) {
            int input_index = n - k;
            if (input_index >= 0) {
                sum_I += I_upsampled[input_index] * mask[k];
                sum_Q += Q_upsampled[input_index] * mask[k];
            }
        }
        
        I_filtred[n] = sum_I;
        Q_filtred[n] = sum_Q;
    }
}

void sdvig(int sample, float I_input[], float Q_input[], float I_output[], float Q_output[], float amplitude){
    for (int i = 0; i < sample; i++){
        //I_filtred[i] = I_filtred[i] * 0.1 * 2047 * 16;  //сдвиг << 4 эквивалентен * 2^4
        //Q_filtred[i] = Q_filtred[i] * 0.1 * 2047 * 16;
        I_output[i] = I_input[i] * amplitude;
        Q_output[i] = Q_input[i] * amplitude;
    }
}

void to_buff(float I_filtred[], float Q_filtred[], int16_t tx_buff[], int sample){
    for (int i = 0; i < sample; i++){
        tx_buff[2*i] = (int16_t)(I_filtred[i] * 10000.0f);
        tx_buff[2*i + 1] = (int16_t)(Q_filtred[i] * 10000.0f);
    }
}

void from_buff(int16_t rx_buff[], float I_received[], float Q_received[], int sample){
    for (int i = 0; i < sample; i++){
        I_received[i] = (float)rx_buff[2*i] / 32767.0f;
        Q_received[i] = (float)rx_buff[2*i + 1] / 32767.0f;
    }
}


void SymbolTimingRecovery(const float I_in[], const float Q_in[], int signal_length, int Nsp, int start_offset, float sym_I[], float sym_Q[], int *symbol_count, int *final_offset) { 
    
    float K1 = 0.05f;
    float K2 = 0.005f;
    float p2 = 0.0f;
    int offset = 0;
    *symbol_count = 0;
    
    int max_symbols = (signal_length - start_offset - 2 * Nsp) / Nsp;
    
    int error_zero_count = 0;
    int error_large_count = 0;
    float min_error = 999.0f;
    float max_error = -999.0f;
    
    for (int sym = 0; sym < max_symbols; sym++) {
        int n = start_offset + sym * Nsp;
        int i_prev = n + offset;
        int i_mid  = n + offset + Nsp / 2;
        int i_next = n + offset + Nsp;
        
        if (i_next >= signal_length) {
            printf("WARNING: i_next (%d) >= signal_length (%d) at sym=%d\n", 
                   i_next, signal_length, sym);
            break;
        }
        
        // Gardner TED
        float e = (I_in[i_next] - I_in[i_prev]) * I_in[i_mid] + 
                  (Q_in[i_next] - Q_in[i_prev]) * Q_in[i_mid];
        
        // Статистика ошибки
        if (e < min_error) min_error = e;
        if (e > max_error) max_error = e;
        if (e > -0.01f && e < 0.01f) error_zero_count++;
        if (e > 1.0f || e < -1.0f) error_large_count++;
        
        // Ограничение ошибки
        if (e > 1.0f) e = 1.0f;
        if (e < -1.0f) e = -1.0f;
        
        // Loop filter
        float p1 = e * K1;
        p2 = p2 + p1 + e * K2;
        
        // Offset
        int prev_offset = offset;
        offset = (int)(roundf(p2 * Nsp)) % Nsp;
        if (offset < 0) offset += Nsp;
        
        // Сохраняем символ
        sym_I[*symbol_count] = I_in[i_mid];
        sym_Q[*symbol_count] = Q_in[i_mid];
        (*symbol_count)++;
        
        // Вывод первых 10 символов и каждого 100-го
        if (sym < 10 || sym % 100 == 0) {
            printf("SYM %3d: n=%5d idx=[%5d,%5d,%5d] I_mid=%.3f Q_mid=%.3f e=%.4f p2=%.4f offset=%d\n",
                   sym, n, i_prev, i_mid, i_next, I_in[i_mid], Q_in[i_mid], e, p2, offset);
        }
        
        // Если offset изменился - выводим
        if (offset != prev_offset && sym >= 10) {
            printf("  >>> OFFSET CHANGED: %d -> %d (p2=%.4f)\n", prev_offset, offset, p2);
        }
    }

    printf("Total symbols captured: %d\n", *symbol_count);
    printf("Final offset: %d (expected 0-%d)\n", offset, Nsp - 1);
    printf("Error range: [%.4f, %.4f]\n", min_error, max_error);
    printf("Near-zero errors (|e|<0.01): %d (%.1f%%)\n", 
           error_zero_count, 100.0f * error_zero_count / (*symbol_count));
    printf("Large errors (|e|>1.0): %d (%.1f%%)\n", 
           error_large_count, 100.0f * error_large_count / (*symbol_count));
    printf("Final p2 (phase): %.4f\n", p2);
    
    if (final_offset != NULL) {
        *final_offset = offset;
    }
}

int main(){
    SoapySDRKwargs args = {};

    SoapySDRKwargs_set(&args, "driver", "plutosdr");        //тип устройства 
    if (1) {
        SoapySDRKwargs_set(&args, "uri", "usb:");           //Способ обмена сэмплами
    } else {
        SoapySDRKwargs_set(&args, "uri", "ip:192.168.2.1"); 
    }

    SoapySDRKwargs_set(&args, "direct", "1");
    SoapySDRKwargs_set(&args, "timestamp_every", "1920");   //Размер буфера + временные метки
    SoapySDRKwargs_set(&args, "loopback", "0");
    SoapySDRDevice *sdr = SoapySDRDevice_make(&args);
    SoapySDRKwargs_clear(&args);

    int sample_rate = 1e6;     // Частота дискретизации 1 МГц
    int carrier_freq = 800e6;  // Несущая частота 800 МГц

    // Параметры RX части
    SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_RX, 0, sample_rate);
    SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_RX, 0, carrier_freq, NULL);
    // Параметры TX части
    SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_TX, 0, sample_rate);
    SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_TX, 0, carrier_freq, NULL);

    // Инициализация количества каналов RX\\TX (один)
    size_t channels[] = {0};

    // Настройки усилителей на RX TX 
    SoapySDRDevice_setGain(sdr, SOAPY_SDR_RX, 0, 50.0);    
    SoapySDRDevice_setGain(sdr, SOAPY_SDR_TX, 0, -10.0);  

    size_t channel_count = sizeof(channels) / sizeof(channels[0]);
    
    // Формирование потоков для передачи и приема сэмплов в формате complex int16
    SoapySDRStream *rxStream = SoapySDRDevice_setupStream(sdr, SOAPY_SDR_RX, SOAPY_SDR_CS16, channels, channel_count, NULL);
    SoapySDRStream *txStream = SoapySDRDevice_setupStream(sdr, SOAPY_SDR_TX, SOAPY_SDR_CS16, channels, channel_count, NULL);

    SoapySDRDevice_activateStream(sdr, rxStream, 0, 0, 0); 
    SoapySDRDevice_activateStream(sdr, txStream, 0, 0, 0); 
    
    // Получение размера буферов
    size_t rx_mtu = SoapySDRDevice_getStreamMTU(sdr, rxStream);
    size_t tx_mtu = SoapySDRDevice_getStreamMTU(sdr, txStream);

    // Параметры сигнала
    const int num_bits = 100;
    const int duration = 10;
    const int num_samples = num_bits * duration;

    int bits[num_bits];
    float I_bpsk[num_bits];
    float Q_bpsk[num_bits];
    float I_upsampled[num_samples];
    float Q_upsampled[num_samples];
    float I_filtred[num_samples];
    float Q_filtred[num_samples];
    float I_amplified[num_samples];
    float Q_amplified[num_samples];
    
    int16_t tx_buff[2 * tx_mtu] = {0}; 
    int16_t rx_buff[2 * rx_mtu] = {0};

    for (int i = 0; i < num_bits; i++){
        bits[i] = rand() % 2;
    }

    // Обработка сигнала для передачи
    to_bpsk(bits, num_bits, I_bpsk, Q_bpsk);
    upsampling(duration, num_bits, I_bpsk, Q_bpsk, I_upsampled, Q_upsampled);
    svertka(num_samples, I_upsampled, Q_upsampled, I_filtred, Q_filtred);
    sdvig(num_samples, I_filtred, Q_filtred, I_amplified, Q_amplified, 0.8f);
    to_buff(I_amplified, Q_amplified, tx_buff, num_samples);

    // Заполнение остатка буфера нулями
    for (int i = num_samples; i < tx_mtu; i++) {
        tx_buff[2*i] = 0;
        tx_buff[2*i + 1] = 0;
    }

    printf("Сгенерировано %d битов, %d сэмплов\n", num_bits, num_samples);

    FILE *rx_file = fopen("rx_samples.pcm", "wb");
    if (!rx_file) {
        perror("Не удалось открыть rx_samples.pcm");
    }

    FILE *tx_file = fopen("tx_samples.pcm", "wb");
    if (!tx_file) {
        perror("Не удалось открыть tx_samples.pcm");
    }

    if (!rx_file || !tx_file) {
        printf("Проблема с открытием файлов\n");
        if (rx_file) fclose(rx_file);
        if (tx_file) fclose(tx_file);
        return 1;
    }

    //первые 8 ячеек для timestamp
    /*for(size_t i = 0; i < 2; i++){
        tx_buff[0 + i] = 0xffff;
        tx_buff[10 + i] = 0xffff;
    }*/

    // Добавляем время, когда нужно передать блок tx_buff, через tx_time -наносекунд
    /*for(size_t i = 0; i < 8; i++)
    {
        uint8_t tx_time_byte = (tx_time >> (i * 8)) & 0xff;
        tx_buff[2 + i] = tx_time_byte << 4;
    }*/

    long long timeNs; //timestamp for receive buffer
    // Переменная для времени отправки сэмплов относительно текущего приема
    long long tx_time = timeNs + (4 * 1000 * 1000); // на 4 [мс] в будущее

    const long timeoutUs = 400000;
    long long last_time = 0;
    int transmission_done = 0;

    for (size_t buffers_read = 0; buffers_read < 10; buffers_read++) {
        void *rx_buffs[] = {rx_buff};
        int flags;        
        long long timeNs; 

        // считали буффер RX, записали его в rx_buffer
        int sr = SoapySDRDevice_readStream(sdr, rxStream, rx_buffs, rx_mtu, &flags, &timeNs, timeoutUs);
        
        if (sr > 0) {
            fwrite(rx_buffs[0], sizeof(int16_t), 2 * sr, rx_file);
            //printf("Buffer: %lu - RX: %i samples\n", buffers_read, sr);
            // Смотрим на количество считаных сэмплов, времени прихода и разницы во времени с чтением прошлого буфера
            printf("Buffer: %lu - Samples: %i, Flags: %i, Time: %lli, TimeDiff: %lli\n", buffers_read, sr, flags, timeNs, timeNs - last_time);
            last_time = timeNs;
        }

        // Переменная для времени отправки сэмплов относительно текущего приема
        long long tx_time = timeNs + (4 * 1000 * 1000); // на 4мс в будущее

        //Передача на третьей итерации
        if (buffers_read == 2 && !transmission_done) {
            void *tx_buffs[] = {tx_buff};
            int tx_flags = SOAPY_SDR_HAS_TIME;
            
            int st = SoapySDRDevice_writeStream(sdr, txStream, tx_buffs, tx_mtu, &tx_flags, tx_time, timeoutUs);
            
            if (st > 0) {
                fwrite(tx_buff, sizeof(int16_t), 2 * st, tx_file);
                printf("TX successful: %d samples\n", st);
                transmission_done = 1;
            }
        }
        
        last_time = timeNs;
    }

    fclose(rx_file);
    fclose(tx_file);

    FILE *rx_input = fopen("rx_samples.pcm", "rb");
    if (!rx_input) {
        printf("Не получилось открыть rx_samples.pcm\n");
        return 1;
    }

    //узнаем размер
    fseek(rx_input, 0, SEEK_END);
    long file_size = ftell(rx_input);
    fseek(rx_input, 0, SEEK_SET);

    int num_rx_samples = file_size / (2 * sizeof(int16_t));
    printf("RX файл содержит %d сэмплов\n", num_rx_samples);

    int16_t *original_rx_data = (int16_t*)malloc(file_size);
    fread(original_rx_data, 1, file_size, rx_input);
    fclose(rx_input);

    //Конвертация int16 в float
    float *rx_I = (float*)malloc(num_rx_samples * sizeof(float));
    float *rx_Q = (float*)malloc(num_rx_samples * sizeof(float));
    from_buff(original_rx_data, rx_I, rx_Q, num_rx_samples);

    //Применение фильтра
    float *filtered_I = (float*)malloc(num_rx_samples * sizeof(float));
    float *filtered_Q = (float*)malloc(num_rx_samples * sizeof(float));
    svertka(num_rx_samples, rx_I, rx_Q, filtered_I, filtered_Q);

    const int samples_per_symbol = duration; // = 10

    ////////////////////////////////////////////////////////////////////
    float *sym_I = (float*)malloc((num_rx_samples / duration) * sizeof(float));
    float *sym_Q = (float*)malloc((num_rx_samples / duration) * sizeof(float));

    int final_offset = 0;
    int start_offset = 0;
    int symbol_count = 0;

    // символьная синхронизация
    SymbolTimingRecovery(filtered_I, filtered_Q, num_rx_samples, duration, start_offset, sym_I, sym_Q, &symbol_count, &final_offset);

    printf("Восстановлено %d символов\n", symbol_count);
    printf("Подобранный сдвиг =  %d \n", final_offset);

    // сдвиг начала на найденный offset
    float *I_shifted = (float*)malloc((num_rx_samples - final_offset) * sizeof(float));
    float *Q_shifted = (float*)malloc((num_rx_samples - final_offset) * sizeof(float));

    for (int i = 0; i < num_rx_samples - final_offset; i++) {
        I_shifted[i] = filtered_I[i + final_offset];
        Q_shifted[i] = filtered_Q[i + final_offset];
    }

    ////////////////////////////////////////////////////////////////////

    //Конвертация float в int16
    int16_t *filtered_sdr = (int16_t*)malloc(2 * num_rx_samples * sizeof(int16_t));
    to_buff(filtered_I, filtered_Q, filtered_sdr, num_rx_samples);

    FILE *rx_output = fopen("rx_filtered.pcm", "wb");
    fwrite(filtered_sdr, sizeof(int16_t), 2 * num_rx_samples, rx_output);
    fclose(rx_output);

    free(original_rx_data);
    free(rx_I);
    free(rx_Q);
    free(filtered_I);
    free(filtered_Q);
    free(I_shifted);
    free(Q_shifted);
    free(filtered_sdr);

    SoapySDRDevice_deactivateStream(sdr, rxStream, 0, 0);
    SoapySDRDevice_deactivateStream(sdr, txStream, 0, 0);
    SoapySDRDevice_closeStream(sdr, rxStream);
    SoapySDRDevice_closeStream(sdr, txStream);
    SoapySDRDevice_unmake(sdr);

    printf("Программа завершена\n");
    return 0;
}