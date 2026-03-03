#include "signal_processing.h" 
#include "imgui_implot.h"
#include "sdr.h"

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

    const int samples_per_symbol = 10;
    const int num_bits = sdr->tx_mtu / samples_per_symbol;  

    vector<int> bits;
    bits.resize(num_bits);
    for (int i = 0; i < num_bits; i++){
        bits[i] = rand() % 2;
    }

    printf("Сгенерировано %zu бит\n", bits.size());

    printf(" BPSK модуляция\n");
    vector<complex<float>> IQ_bpsk = to_bpsk(bits);
    printf("Получено %zu символов\n", IQ_bpsk.size());

    printf(" Upsampling\n");
    vector<complex<float>> IQ_upsampled = upsampling(IQ_bpsk, num_bits, samples_per_symbol);
    printf("Получено %zu сэмплов\n", IQ_upsampled.size());

    printf(" Свертка\n");
    vector<complex<float>> IQ_convolved = convolve(IQ_upsampled, samples_per_symbol);
    printf("Получено %zu сэмплов\n", IQ_convolved.size());

    printf(" ОБРАБОТКА ЗАКОНЧЕНА \n");

    // конвертация в буфер
    int16_t *tx_buff = (int16_t*)malloc(2*sdr->tx_mtu*sizeof(int16_t));
    to_buff(IQ_convolved, tx_buff, IQ_convolved.size());

    // заполняем конец нулями
    for (size_t i = IQ_convolved.size(); i < sdr->tx_mtu; i++) {
        tx_buff[2*i] = 0;
        tx_buff[2*i + 1] = 0;
    }
    
    int16_t *rx_buff = (int16_t*)malloc(2 * sdr->rx_mtu * sizeof(int16_t));
    long long timeNs = 0; 
    const long timeoutUs = 400000;

    FILE *rx_file = fopen("rx_samples.pcm", "wb");
    if (!rx_file) {
        perror("Не удалось открыть rx_samples.pcm");
    }

    FILE *tx_file = fopen("tx_samples.pcm", "wb");
    if (!tx_file) {
        perror("Не удалось открыть tx_samples.pcm");
    }
    
    vector<complex<float>> rx_signal;

    size_t iteration_count = 20;
    printf("НАЧАЛО ПРИЕМА/ПЕРЕДАЧИ\n");

    for (size_t i = 0; i < iteration_count; i++)  {
        
        // ПРИЁМ
        void *rx_buffs[] = {rx_buff};
        int flags;
        // считали буффер RX, записали его в rx_buffer
        int sr = SoapySDRDevice_readStream(sdr->device, sdr->rxStream, rx_buffs, sdr->rx_mtu, &flags, &timeNs, timeoutUs);
        
        if (sr > 0) {
            printf("Приём %zu: %d сэмплов\n", i, sr);
            
            if (rx_file != NULL) {
                fwrite(rx_buff, sizeof(int16_t), 2 * sr, rx_file);
            }
            
            vector<complex<float>> rx_complex;
            from_buff(rx_complex, rx_buff, sr);
            rx_signal.insert(rx_signal.end(), rx_complex.begin(), rx_complex.end());
        } else {
            printf("Приём %zu: ОШИБКА (%d)\n", i, sr);
        }

        // передача на 3 итерации
        if (i == 2) {
            printf("\n  ОТПРАВКА СИГНАЛА (%zu сэмплов) <<<\n", IQ_convolved.size());
            
            if (tx_file != NULL) {
                fwrite(tx_buff, sizeof(int16_t), 2 * sdr->tx_mtu, tx_file);
            }
            
            long long tx_time = timeNs + (4 * 1000 * 1000);
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
        
        this_thread::sleep_for(chrono::milliseconds(50));
    }

    if (rx_file != NULL) fclose(rx_file);
    if (tx_file != NULL) fclose(tx_file);
    printf("\n Файлы сохранены \n");

    printf("\n  ОБРАБОТКА ПРИНЯТЫХ ДАННЫХ \n");

    if (rx_signal.size() > 0) {
        printf("Всего получено сэмплов: %zu\n", rx_signal.size());
        
        vector<complex<float>> IQ_convolved2 = convolve(IQ_convolved, samples_per_symbol);
        vector<float> erof = symbol_sync(IQ_convolved2, samples_per_symbol);
        vector<complex<float>> IQ_downsampled = downsampling(erof, IQ_convolved2);
        vector<int> bits2 = from_bpsk(IQ_downsampled);
        
        printf("Точек синхронизации: %zu\n", erof.size());
        printf("Восстановлено символов: %zu\n", IQ_downsampled.size());
        printf("\n ОБРАБОТКА ЗАВЕРШЕНА \n");
        
        run_gui(bits, IQ_bpsk, IQ_upsampled, IQ_convolved, IQ_convolved2, IQ_downsampled, bits2, erof, samples_per_symbol);

    } else {
        printf(" ПРЕДУПРЕЖДЕНИЕ: Данные приёма пусты\n");
    }
    
    free(tx_buff);
    free(rx_buff);
    sdr_cleanup(sdr);
    cout << "\n РАБОТА ЗАВЕРШЕНА \n" << endl;

    return 0;
  
}