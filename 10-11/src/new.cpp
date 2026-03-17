#include "signal_processing.h" 
#include "imgui_implot.h"
#include "sdr.h"
#include "test_rx_samples_bpsk_barker13.h"
#include <cmath>
#include <complex>

using namespace std;

int main(){

    srand((unsigned int)time(NULL));
    
    sdr_device_t *sdr = sdr_init(1);
    if (sdr == NULL) {
        printf("ОШИБКА: Не удалось инициализировать SDR устройство\n");
        return 1;
    }
    if (sdr_configure(sdr) != 0) { sdr_cleanup(sdr); return 1; }

    printf(" УСТРОЙСТВО ИНИЦИАЛИЗИРОВАНО \n");
    printf("TX MTU - %zu сэмплов\n", sdr->tx_mtu);

    int samples_per_symbol = 10;
    const int num_bits = sdr->tx_mtu / samples_per_symbol - 7;  

    vector<int> bits;
    bits.resize(num_bits);
    for (int i = 0; i < num_bits; i++){
        bits[i] = rand() % 2;
    }

    printf("Сгенерировано %zu бит\n", bits.size());

    printf(" BPSK модуляция\n");
    vector<complex<float>> IQ_bpsk = to_bpsk(bits);
    printf("Получено %zu символов\n", IQ_bpsk.size());

    printf(" Добавление кода Баркера\n");
    vector<complex<float>> IQ_barker = barker_code(IQ_bpsk);
  //  vector<complex<float>> barker = {(1.0f, 0.0f),(1.0f, 0.0f),(1.0f, 0.0f),(-1.0f, 0.0f),(-1.0f, 0.0f),(1.0f, 0.0f),(-1.0f, 0.0f)};
    printf("Получено %zu символов\n", IQ_barker.size());

    printf(" Upsampling\n");
    vector<complex<float>> IQ_upsampled = upsampling(IQ_bpsk, samples_per_symbol);
   // vector<complex<float>> barker_upsampled = upsampling(barker, samples_per_symbol);
    printf("Получено %zu сэмплов\n", IQ_upsampled.size());

    printf(" Свертка\n");
    vector<complex<float>> IQ_convolved = convolve(IQ_upsampled, samples_per_symbol);
    printf("Получено %zu сэмплов\n", IQ_convolved.size());

    printf(" ОБРАБОТКА ЗАКОНЧЕНА \n");

    // конвертация в буфер
    printf(" Конвертация в буфер\n");

    int16_t *tx_buff = (int16_t*)malloc(2 * sdr->tx_mtu * sizeof(int16_t)); 
    
    to_buff(IQ_convolved, tx_buff, IQ_convolved.size());

    //первые 8 ячеек для timestamp
    for(size_t i = 0; i < 2; i++){
        tx_buff[0 + i] = 0xffff;
        tx_buff[10 + i] = 0xffff;
    }

    printf("\nБуфер передачи готов: %zu сэмплов\n", IQ_convolved.size());

    // заполняем конец нулями
    // for (size_t i = IQ_convolved.size(); i < sdr->tx_mtu; i++) {
    //     tx_buff[2*i] = 0;
    //     tx_buff[2*i + 1] = 0;
    // }
    
    int16_t *rx_buff = (int16_t*)malloc(2 * sdr->rx_mtu * sizeof(int16_t));
    long long timeNs = 0; 
    const long timeoutUs = 400000;

    FILE *rx_file = fopen("rx_samples.pcm", "wb");
    if (!rx_file) {
        perror("Не удалось открыть rx_samples.pcm");
    }
    vector<complex<float>> rx_complex(1920);
    
    FILE *tx_file = fopen("tx_samples.pcm", "wb");
    if (!tx_file) {
        perror("Не удалось открыть tx_samples.pcm");
    }
    
    vector<complex<float>> rx_signal;

    size_t iteration_count = 100;
    printf("НАЧАЛО ПРИЕМА/ПЕРЕДАЧИ\n");

    for (size_t i = 0; i < iteration_count; i+=2)  {
        
        // ПРИЁМ
        void *rx_buffs[] = {rx_buff};
        int flags;
        // считали буффер RX, записали его в rx_buffer
        int sr = SoapySDRDevice_readStream(sdr->device, sdr->rxStream, rx_buffs, sdr->rx_mtu, &flags, &timeNs, timeoutUs);
        
        if (sr > 0) {
            printf("Приём %zu: %d сэмплов\n", i, sr);
            
            if (rx_file != NULL && sr > 0) {

                fwrite(rx_buff, sizeof(int16_t), 2 * sr, rx_file);
            }
            rx_complex.insert(rx_complex.end(),rx_buff, rx_buff + 2*sr);
        } 
    
        // передача на 3 итерации
        if (sr > 0) {
            printf("\n  ОТПРАВКА СИГНАЛА (%zu сэмплов)\n", IQ_convolved.size());
            
            if (tx_file != NULL) {
                size_t written = fwrite(tx_buff, sizeof(int16_t), 2 * IQ_convolved.size(), tx_file);
                if (written != 2 * IQ_convolved.size()) {
                    perror("Ошибка записи tx_samples.pcm");
                }
            }
            long long tx_time = timeNs + (10 * 1000 * 1000);  //10 мс

            for(size_t j = 0; j < 8; j++) {
                uint8_t tx_time_byte = (tx_time >> (j * 8)) & 0xff;
                tx_buff[2 + j] = tx_time_byte << 4;
            }
            
            void *tx_buffs[] = {tx_buff};
            int tx_flags = SOAPY_SDR_HAS_TIME;
            int st = SoapySDRDevice_writeStream(sdr->device, sdr->txStream, (const void * const*)tx_buffs, sdr->tx_mtu, &tx_flags, tx_time, timeoutUs);
            
            if (st != (int)sdr->tx_mtu) {
                printf("    ОШИБКА передачи: %d\n", st);
            } else {
                printf(" Успешно отправлено\n");
            }
        }
    
    }

    if (rx_file != NULL) fclose(rx_file);
    if (tx_file != NULL) fclose(tx_file);
    printf("\n Файлы сохранены \n");

    printf("\n  ОБРАБОТКА ПРИНЯТЫХ ДАННЫХ \n");

    if (test_rx_samples_bpsk_barker13.size() > 0) {
        printf("Всего получено сэмплов: %zu\n", test_rx_samples_bpsk_barker13.size());
        samples_per_symbol = 16;

        const size_t start_idx = 650;
        const size_t end_idx = start_idx + 1920;
        
        if (end_idx > test_rx_samples_bpsk_barker13.size()) {
            printf(" ПРЕДУПРЕЖДЕНИЕ: end_idx=%zu превышает размер данных (%zu)\n", 
                end_idx, test_rx_samples_bpsk_barker13.size());
        }

        // ОБРЕЗАЕМ ДО 1 БУФФЕРА
        vector<complex<float>> rx_subset;

        for (int i = start_idx; i < (int)end_idx; i++){
            rx_subset.push_back(test_rx_samples_bpsk_barker13[i] / (float)pow(2,11));
        }
       
        int syms = 5;
        float beta = 0.75;
        vector<float> filter = srrc(syms, beta, samples_per_symbol, 0.0f);
        vector<complex<float>> IQ_matched = convolve2(rx_subset, filter);
        //vector<complex<float>> IQ_matched = matched_filter(rx_subset, samples_per_symbol);

        //vector<float> erof = symbol_sync(IQ_matched, samples_per_symbol);
        vector<complex<float>> IQ_symbol_sync = clock_recovery_mueller_muller(IQ_matched, samples_per_symbol);
        //vector<complex<float>> IQ_downsampled = downsampling(erof, IQ_matched);

        vector<complex<float>> IQ_freq = freq_sync_bpsk(IQ_symbol_sync);

        cout << "Формируем код Баркера (13 бит)" << endl;

        vector<complex<float>> barker_complex = {{1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f},
        {-1.0f, 0.0f}, {-1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f}, {-1.0f, 0.0f},
        {1.0f, 0.0f}, {-1.0f, 0.0f}, {1.0f, 0.0f}};
        
        float threshold_ratio = 5.5f;

        vector<complex<float>> znach_corr = cross_correlation(IQ_freq, barker_complex);

        // for (int i = 0; i < (int)znach_corr.size(); i++){
        //     cout << znach_corr[i];
        // }

        // ПОИСК НАЧАЛА КОДА БАРКЕРА
        int barker_index = find_barker(IQ_freq, barker_complex, threshold_ratio);
        printf("Начало кода баркера: %d\n", barker_index);
        
        // УБИРАЕМ БАРКЕРА
        vector<complex<float>> data_only(IQ_freq.begin() + barker_index + 13, IQ_freq.end());

        vector<int> bits2 = from_bpsk(data_only);

        for (int i = 0; i < (int)bits2.size(); i++){
             cout << bits2[i];
         }
        
        printf("\n ОБРАБОТКА ЗАВЕРШЕНА \n");
        
        run_gui(bits, IQ_bpsk, IQ_upsampled, IQ_convolved, rx_subset, IQ_matched, IQ_symbol_sync, IQ_freq, znach_corr, data_only, bits2, samples_per_symbol);

    } else {
        printf(" ПРЕДУПРЕЖДЕНИЕ: Данные приёма пусты\n");
    }
    
    free(tx_buff);
    free(rx_buff);
    sdr_cleanup(sdr);
    cout << "\n РАБОТА ЗАВЕРШЕНА \n" << endl;

    return 0;
}