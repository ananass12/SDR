import numpy as np
import matplotlib.pyplot as plt

data = np.fromfile("rx_samples.pcm", dtype=np.int16)

I = data[0::2]  
Q = data[1::2]  

plt.figure(figsize=(15, 10))

plt.subplot(2, 2, 1)
plt.plot(I, label='I (Real)', linewidth=0.8, alpha=0.7)
plt.plot(Q, label='Q (Imag)', linewidth=0.8, alpha=0.7)
plt.xlabel('Индекс сэмпла')
plt.ylabel('Амплитуда')
plt.title('Весь принятый сигнал')
plt.legend()
plt.grid(True)

plt.subplot(2, 2, 2)
plt.plot(I, label='I (Real)', linewidth=1.2)
plt.plot(Q, label='Q (Imag)', linewidth=1.2)
plt.xlabel('Индекс сэмпла')
plt.ylabel('Амплитуда')
plt.title('Принятый сигнал (близко)')
plt.xlim(7800, 9000)
plt.legend()
plt.grid(True)

data2 = np.fromfile("rx_filtered.pcm", dtype=np.int16)

I2 = data2[0::2]  
Q2 = data2[1::2]  

plt.subplot(2, 2, 3)
plt.plot(I2, label='I (Real)', linewidth=0.8, alpha=0.7)
plt.plot(Q2, label='Q (Imag)', linewidth=0.8, alpha=0.7)
plt.xlabel('Индекс сэмпла')
plt.ylabel('Амплитуда')
plt.title('Весь отфильтрованный сигнал')
plt.legend()
plt.grid(True)

plt.subplot(2, 2, 4)
plt.plot(I2, label='I (Real)', linewidth=1.5, alpha=0.8)
plt.plot(Q2, label='Q (Imag)', linewidth=1.5, alpha=0.8)
plt.xlabel('Индекс сэмпла')
plt.ylabel('Амплитуда')
plt.xlim(7800, 9000)
plt.title('Пилообразный сигнал (близко)')
plt.legend()
plt.grid(True)

plt.tight_layout()
plt.show()