import numpy as np
import matplotlib.pyplot as plt

data = np.fromfile("rxdata.pcm", dtype=np.int16)

I = data[0::2]  
Q = data[1::2]  

plt.figure(figsize=(12, 6))
plt.plot(I, label='I (Real)', linewidth=0.8)
plt.plot(Q, label='Q (Imag)', linewidth=0.8)
plt.xlabel('Индекс сэмпла')
plt.ylabel('Амплитуда')
plt.title('Полученный сигнал')
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()