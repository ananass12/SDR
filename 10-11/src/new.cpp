#include "signal_processing.h"
#include "imgui_implot.h"
#include "sdr.h"
#include "test_rx_samples_bpsk_barker13.h"
#include <cmath>
#include <complex>
#include <fftw3.h> 
#include <vector>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

using namespace std;
atomic<bool> running(true);
using std::vector;
using std::complex;

int main(){

   srand((unsigned int)time(NULL));

   // ИНИЦИАЛИЗАЦИЯ SDR
  
   sdr_device_t *sdr_tx = sdr_init(1);
   sdr_device_t *sdr_rx = sdr_init(2);

   if (sdr_tx == NULL || sdr_rx == NULL) {
       printf("ОШИБКА: Не удалось инициализировать SDR устройство\n");
       return 1;
   }

   if (sdr_configure(sdr_tx) != 0) return 1;
   if (sdr_configure(sdr_rx) != 0) return 1;

   printf(" УСТРОЙСТВО ИНИЦИАЛИЗИРОВАНО \n");
   printf("TX MTU - %zu сэмплов\n", sdr_tx->tx_mtu);

   // ДАННЫЕ

   int sample_per_symbol = 16;
   const int num_bits = sdr_tx->tx_mtu / sample_per_symbol - 7; 

   vector<int> bits;
   bits.resize(num_bits);
   for (int i = 0; i < num_bits; i++){
       bits[i] = rand() % 2;
   }

   vector<int> hello_sibguti = {0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0,
        0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1,
        1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1,
        1, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0,
        0, 0, 0, 1, 1, 0, 0, 0, 0};

   printf("Сгенерировано %zu бит\n", hello_sibguti.size());

    // ОБРАБОТКА ДАННЫХ

   printf(" BPSK модуляция\n");
   vector<complex<float>> IQ_bpsk = to_bpsk(hello_sibguti);
   printf("Получено %zu символов\n", IQ_bpsk.size());

   printf(" Добавление кода Баркера\n");
   vector<complex<float>> IQ_barker = barker_code(IQ_bpsk);
   printf("Получено %zu символов\n", IQ_barker.size());

   printf(" OFDM\n");
   vector<complex<float>> IQ_ofdm = OFDM_Modulate(IQ_bpsk);

   printf(" OFDM DEMO\n");
   vector<complex<float>> IQ_ofdm_demodulate = OFDM_Demodulate(IQ_ofdm);

   vector<int> biiits;
   biiits = from_bpsk(IQ_ofdm_demodulate);

   for (int i = 0; i < (int)biiits.size(); i++) {
       cout << biiits[i];
   }
   cout << endl;


   //printf(" Upsampling\n");
   //vector<complex<float>> IQ_upsampled = upsampling(IQ_bpsk, sample_per_symbol);
   //vector<complex<float>> barker_upsampled = upsampling(barker, samples_per_symbol);
   //printf("Получено %zu сэмплов\n", IQ_upsampled.size());

   //printf(" Свертка\n");
   //vector<complex<float>> IQ_convolved = convolve(IQ_upsampled, sample_per_symbol);
   //printf("Получено %zu сэмплов\n", IQ_convolved.size());

   printf(" ОБРАБОТКА ЗАКОНЧЕНА \n");

   // БУФФЕРЫ

   printf(" Конвертация в буфер\n");

    int16_t *tx_buff = (int16_t*)malloc(2 * sdr_tx->tx_mtu * sizeof(int16_t));
    int16_t *rx_buff = (int16_t*)malloc(2 * sdr_rx->rx_mtu * sizeof(int16_t));

    vector<complex<float>> rx_complex;
    rx_complex.reserve(200000);

    long long base_time = 0;
    long long timeNs = 0;
    const long timeoutUs = 400000;

    FILE *rx_file = fopen("rx_samples.pcm", "wb");
    FILE *tx_file = fopen("tx_samples.pcm", "wb");

    if (!tx_file) {
        perror("Не удалось открыть tx_samples.pcm");
    }
    
    if (!rx_file) {
        perror("Не удалось открыть rx_samples.pcm");
    }
  


  
   to_buff(IQ_ofdm, tx_buff, IQ_ofdm.size());

   //первые 8 ячеек для timestamp
   for(size_t i = 0; i < 2; i++){
       tx_buff[0 + i] = 0xffff;
       tx_buff[10 + i] = 0xffff;
   }

   printf("\nБуфер передачи готов: %zu сэмплов\n", IQ_ofdm.size());

   // заполняем конец нулями
   // for (size_t i = IQ_convolved.size(); i < sdr->tx_mtu; i++) {
   //     tx_buff[2*i] = 0;
   //     tx_buff[2*i + 1] = 0;
   // }
  
   
   vector<complex<float>> rx_signal;

   size_t iteration_count = 100;
   

   // ПОТОК RX

   thread rx_thread([&]() {
        while (running) {
            void *rx_buffs[] = {rx_buff};
            int flags;
            long long timeNs;

            int sr = SoapySDRDevice_readStream(
                sdr_rx->device,
                sdr_rx->rxStream,
                rx_buffs,
                sdr_rx->rx_mtu,
                &flags,
                &timeNs,
                timeoutUs
            );

            if (sr > 0) {
                if (base_time == 0)
                    base_time = timeNs;
                if (rx_file)
                    fwrite(rx_buff, sizeof(int16_t), 2 * sr, rx_file);
                for (int i = 0; i < sr; i++) {
                    float I_ = rx_buff[2*i] / 2048.0f;
                    float Q = rx_buff[2*i + 1] / 2048.0f;
                    rx_complex.emplace_back(I_, Q);
                }
            }
        }
    });

    // ПОТОК TX

    thread tx_thread([&]() {

        while (running) {

            size_t tx_len = min(IQ_ofdm.size(), (size_t)sdr_tx->tx_mtu);
            to_buff(IQ_ofdm, tx_buff, tx_len);

            long long tx_time = base_time + 10e6;

            if (tx_file)
                fwrite(tx_buff, sizeof(int16_t), 2 * tx_len, tx_file);

            void *tx_buffs[] = {tx_buff};

            int flags = SOAPY_SDR_HAS_TIME;

            int st = SoapySDRDevice_writeStream(
                sdr_tx->device,
                sdr_tx->txStream,
                (const void * const*)tx_buffs,
                tx_len,            
                &flags,              
                tx_time,
                timeoutUs
            );

            this_thread::sleep_for(chrono::milliseconds(10));
        }
    });

    // ОСНОВНОЙ ПОТОК

    printf("НАЧАЛО ПРИЕМА/ПЕРЕДАЧИ\n");

    for (size_t i = 0; i < 100; i++) {
        this_thread::sleep_for(chrono::milliseconds(50));
    }

    // ОСТАНОВКА

    running = false;

    rx_thread.join();
    tx_thread.join();

    if (rx_file) fclose(rx_file);
    if (tx_file) fclose(tx_file);

    free(tx_buff);
    free(rx_buff);

    sdr_cleanup(sdr_tx);
    sdr_cleanup(sdr_rx);

    printf("ЗАВЕРШЕНО\n");

    // ОБРАБОТКА ПРИНЯТЫХ ДАННЫХ

    printf("\n  ОБРАБОТКА ПРИНЯТЫХ ДАННЫХ \n");

   if (test_rx_samples_bpsk_barker13.size() > 0) {
        printf("Всего сэмплов: %zu\n", test_rx_samples_bpsk_barker13.size());

        vector<complex<float>> rx_full;
        rx_full.reserve(test_rx_samples_bpsk_barker13.size());
        for (size_t i = 0; i < test_rx_samples_bpsk_barker13.size(); i++)
            rx_full.push_back(test_rx_samples_bpsk_barker13[i] / 2048.0f);

        int syms = 5; float beta = 0.75f;
        vector<float> filter = srrc(syms, beta, sample_per_symbol, 0.0f);
        vector<complex<float>> IQ_matched = convolve2(rx_full, filter);
        vector<complex<float>> IQ_symbol_sync = clock_recovery_mueller_muller(IQ_matched, sample_per_symbol);
        vector<complex<float>> IQ_freq = freq_sync_bpsk(IQ_symbol_sync);


        vector<complex<float>> IQ_freq_cyclic = IQ_freq;
        int barker_len = 13;
        IQ_freq_cyclic.insert(IQ_freq_cyclic.end(), IQ_freq.begin(), IQ_freq.begin() + barker_len);

        // Код Баркера (13 бит)
        vector<complex<float>> barker_complex = {
            {1,0},{1,0},{1,0},{1,0},{1,0},{-1,0},{-1,0},{1,0},{1,0},{-1,0},{1,0},{-1,0},{1,0}
        };
        float threshold = 5.0f;

        // ПОИСК ВСЕХ ВХОЖДЕНИЙ КОДА БАРКЕРА
        vector<complex<float>> znach_corr = cross_correlation(IQ_freq, barker_complex);

        vector<int> barker_positions;  // позиции всех найденных кодов Баркера
        float min_dist = 50;           // мин. расстояние между кодами (защита от ложных срабатываний)

        for (int i = 0; i < (int)znach_corr.size(); i++) {
            if (znach_corr[i].real() >= threshold) {
                // Проверяем, не слишком ли близко к предыдущему найденному
                bool too_close = false;
                for (int pos : barker_positions) {
                    if (abs(i - pos) < min_dist) { too_close = true; break; }
                }
                if (!too_close) {
                    barker_positions.push_back(i);
                    printf(" - Код Баркера найден на позиции %d (корреляция: %.2f)\n", 
                        i, znach_corr[i].real());
                }
            }
        }

        printf("\n Обнаружено фреймов: %zu\n", barker_positions.size());

        // ОБРАБОТКА КАЖДОГО ФРЕЙМА
        for (size_t f = 0; f < barker_positions.size(); f++) {
            int start = barker_positions[f] + 13;  // после кода Баркера
            int frame_len = 100;  // ожидаемая длина данных в одном фрейме
            
            if (start + frame_len > (int)IQ_freq.size()) {
                printf(" !!! Фрейм %zu: недостаточно данных (начало=%d, доступно=%zu)\n", 
                    f+1, start, IQ_freq.size() - start);
                continue;
            }
            
            // Извлекаем данные фрейма
            vector<complex<float>> frame_data(IQ_freq.begin() + start, IQ_freq.begin() + start + frame_len);
            vector<int> frame_bits = from_bpsk(frame_data);
            
            printf("Фрейм %zu: ", f+1);
            for (int b : frame_bits) cout << b;
            cout << endl;
        }

        vector<complex<float>> data_only;
        vector<int> bits2;
        int barker_index = find_barker(IQ_freq_cyclic, barker_complex, threshold);
        
        if (barker_index >= 0) {
            // Извлечение данных после Баркера
            int start = barker_index + 13;
            int take = 100;
            for (int k = 0; k < take && (start + k) < (int)IQ_freq_cyclic.size(); k++)
                data_only.push_back(IQ_freq_cyclic[start + k]);
            
            bits2 = from_bpsk(data_only);
            printf("Принято бит: ");
            for (int b : bits2) cout << b;
            cout << endl;
        } else {
            printf("Код Баркера не найден (попробуйте уменьшить порог: сейчас %.1f)\n", threshold);
        }

      
       printf("\n ОБРАБОТКА ЗАВЕРШЕНА \n");
      
       run_gui(hello_sibguti, IQ_bpsk, IQ_ofdm, IQ_ofdm_demodulate, rx_full, IQ_matched, IQ_symbol_sync, IQ_freq, znach_corr, data_only, bits2, sample_per_symbol);

   } else {
       printf(" ПРЕДУПРЕЖДЕНИЕ: Данные приёма пусты\n");
   }
  
   free(tx_buff);
   free(rx_buff);
   sdr_cleanup(sdr_tx);
   sdr_cleanup(sdr_rx);
   cout << "\n РАБОТА ЗАВЕРШЕНА \n" << endl;

   return 0;
}
