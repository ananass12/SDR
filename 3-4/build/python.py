import numpy as np
import matplotlib.pyplot as plt

name = "txdata.pcm"

data = []
imag = []
real = []
count = []
counter = 0

with open(name, "rb") as f:
    index = 0
    while (byte := f.read(2)):
        if index % 2 == 0:
            real.append(int.from_bytes(byte, byteorder='little', signed=True))
            counter += 1
            count.append(counter)
        else:
            imag.append(int.from_bytes(byte, byteorder='little', signed=True))
        index += 1

plt.figure(1)
plt.plot(count, real, color='blue', label='Real (I)')
plt.plot(count, imag, color='red', label='Imag (Q)')
plt.xlabel('Samples')
plt.ylabel('Amplitude')
plt.legend()
plt.grid(True)
plt.show()