#include "signal_processing.h"
#include "imgui_implot.h"
#include "sdr.h"
#include "test_rx_samples_bpsk_barker13.h"
#include <cmath>
#include <complex>
#include <fftw3.h> 

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

   int sample_per_symbol = 16;
   const int num_bits = sdr->tx_mtu / sample_per_symbol - 7; 

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

   printf(" Конвертация в буфер\n");

   int16_t *tx_buff = (int16_t*)malloc(2 * sdr->tx_mtu * sizeof(int16_t));
  
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
           //printf("\n  ОТПРАВКА СИГНАЛА (%zu сэмплов)\n", IQ_ofdm.size());
          
           if (tx_file != NULL) {
               size_t written = fwrite(tx_buff, sizeof(int16_t), 2 * IQ_ofdm.size(), tx_file);
               if (written != 2 * IQ_ofdm.size()) {
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
               //printf("    ОШИБКА передачи: %d\n", st);
           } else {
               //printf(" Успешно отправлено\n");
           }
       }
  
   }

   if (rx_file != NULL) fclose(rx_file);
   if (tx_file != NULL) fclose(tx_file);
   printf("\n Файлы сохранены \n");

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
   sdr_cleanup(sdr);
   cout << "\n РАБОТА ЗАВЕРШЕНА \n" << endl;

   return 0;
}
