#include <SoapySDR/Device.h>   //Инициализация устройства
#include <SoapySDR/Formats.h>  //Типы данных, используемых для записи сэмплов
#include <stdio.h>             //printf
#include <stdlib.h>            //free
#include <stdint.h>
#include <complex.h>

// Чтение из файла 
int16_t *read_pcm(const char *filename, size_t *sample_count)
{
    FILE *file = fopen(filename, "rb");
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    printf("размер файла = %ld\n", file_size);
    
    *sample_count = file_size / sizeof(int16_t);
    int16_t *samples = (int16_t *)malloc(file_size);
    size_t sf = fread(samples, sizeof(int16_t), *sample_count, file);
    
    if (sf == 0){
        printf("файл %s пустой", filename);
    }
    
    fclose(file);
    
    return samples;
}

int main(){

SoapySDRKwargs args = {};
    SoapySDRKwargs_set(&args, "driver", "plutosdr");
    SoapySDRKwargs_set(&args, "uri", "usb:");
    SoapySDRKwargs_set(&args, "direct", "1");
    SoapySDRKwargs_set(&args, "timestamp_every", "1920");
    SoapySDRKwargs_set(&args, "loopback", "0");
    SoapySDRDevice *sdr = SoapySDRDevice_make(&args);
    SoapySDRKwargs_clear(&args);

int sample_rate = 1e6;
int carrier_freq = 800e6;

// Параметры RX части
SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_RX, 0, sample_rate);
SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_RX, 0, carrier_freq , NULL);

// Параметры TX части
SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_TX, 0, sample_rate);
SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_TX, 0, carrier_freq , NULL);

// Инициализация количества каналов RX\\TX (в AdalmPluto он один, нулевой)
size_t channels[] = {0};
// Настройки усилителей на RX\\TX    n   m
SoapySDRDevice_setGain(sdr, SOAPY_SDR_RX, 0, 40.0);
    SoapySDRDevice_setGain(sdr, SOAPY_SDR_TX, 0, -7.0);

size_t channel_count = sizeof(channels) / sizeof(channels[0]);
// Формирование потоков для передачи и приема сэмплов
SoapySDRStream *rxStream = SoapySDRDevice_setupStream(sdr, SOAPY_SDR_RX, SOAPY_SDR_CS16, channels, channel_count, NULL);
SoapySDRStream *txStream = SoapySDRDevice_setupStream(sdr, SOAPY_SDR_TX, SOAPY_SDR_CS16, channels, channel_count, NULL);

SoapySDRDevice_activateStream(sdr, rxStream, 0, 0, 0); //start streaming
SoapySDRDevice_activateStream(sdr, txStream, 0, 0, 0); //start streaming
// Получение MTU (Maximum Transmission Unit), в нашем случае - размер буферов. 
size_t rx_mtu = SoapySDRDevice_getStreamMTU(sdr, rxStream);
size_t tx_mtu = SoapySDRDevice_getStreamMTU(sdr, txStream);

// Чтение PCM файла
const char *pcm_filename = "audio.pcm";
size_t sample_count = 0;
int16_t *samples = read_pcm(pcm_filename, &sample_count);

size_t samples_per_buffer = tx_mtu * 2;
size_t num_buffers = sample_count / samples_per_buffer;
size_t remainder = sample_count % samples_per_buffer;

if (remainder > 0) {
    num_buffers++;
}

int16_t *rx_buffer = (int16_t *)malloc(rx_mtu * 2 * sizeof(int16_t));
void *rx_buffs[] = {rx_buffer};
FILE *rx_file = fopen("audio2.pcm", "wb");

const long timeoutUs = 400000;
long long last_time = 0;

printf("Сэмплы: %zu, Буфер: %zu\n", sample_count, num_buffers);

// Основной цикл передачи/приема
for (size_t buffer_idx = 0; buffer_idx < num_buffers; buffer_idx++)
{
    int flags = 0;
    long long timeNs = 0;
        
    // Прием данных
    int sr = SoapySDRDevice_readStream(sdr, rxStream, rx_buffs, rx_mtu, &flags, &timeNs, timeoutUs);
        
    if (sr > 0) {
        fwrite(rx_buffer, sizeof(int16_t), sr * 2, rx_file);
    }
        
    // Передача данных
    size_t current_samples = (buffer_idx == num_buffers - 1 && remainder > 0) ? 
        remainder : samples_per_buffer;
        
    const void *tx_buff = samples + (buffer_idx * samples_per_buffer);
        
    int tx_flags = SOAPY_SDR_HAS_TIME;
    long long tx_time = timeNs + (4 * 1000 * 1000);
        
    int st = SoapySDRDevice_writeStream(sdr, txStream, &tx_buff, 
        current_samples / 2,
        &tx_flags, tx_time, timeoutUs);
        
    printf("Буфер: %zu - RX: %i, TX: %i\n", buffer_idx, sr, st);
        
    last_time = tx_time;
}

if (rx_file) fclose(rx_file);

// Остановка потоков
SoapySDRDevice_deactivateStream(sdr, rxStream, 0, 0);
SoapySDRDevice_deactivateStream(sdr, txStream, 0, 0);

// Закрытие потоков
SoapySDRDevice_closeStream(sdr, rxStream);
SoapySDRDevice_closeStream(sdr, txStream);

// Освобождение устройства
SoapySDRDevice_unmake(sdr);

return 0;
}