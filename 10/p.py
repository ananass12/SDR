import numpy as np
import matplotlib.pyplot as plt
e = np.loadtxt("ted_error.txt")
plt.plot(e, 'o-')
plt.title("Ошибка тайминга по Гарднеру")
plt.xlabel("Номер символа")
plt.ylabel("e[n]")
plt.grid(True)
plt.show()