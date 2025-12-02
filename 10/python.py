import numpy as np
import matplotlib.pyplot as plt

data = np.fromfile("rx_samples.pcm", dtype=np.int16)

I = data[0::2]  
Q = data[1::2]  

plt.figure(figsize=(15, 10))

plt.subplot(3, 1, 1)
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

plt.subplot(3, 1, 2)
plt.plot(I2, label='I (Real)', linewidth=1.5, alpha=0.8)
plt.plot(Q2, label='Q (Imag)', linewidth=1.5, alpha=0.8)
plt.xlabel('Индекс сэмпла')
plt.ylabel('Амплитуда')
plt.xlim(7800, 9000)
plt.title('Пилообразный сигнал (близко)')
plt.legend()
plt.grid(True)


step = 10
start_idx = 7906
end_idx = 8807

I2_subsampled = I2[start_idx:end_idx:step]
Q2_subsampled = Q2[start_idx:end_idx:step]

plt.subplot(3, 1, 3)
plt.scatter(I2_subsampled, Q2_subsampled, s=20, alpha=0.6, color='blue', marker='o')
plt.xlabel('I')
plt.ylabel('Q')
plt.title(f'Созвездие БПСК (каждый {step}-й отсчет, всего {len(I2_subsampled)} точек)')
plt.grid(True)
plt.axhline(y=0, color='black', linewidth=0.5, alpha=0.5)
plt.axvline(x=0, color='black', linewidth=0.5, alpha=0.5)
plt.axis('equal')
plt.tight_layout()
plt.show()