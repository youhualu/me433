from ulab import numpy as np
import time
import random

x = np.linspace(0, 500, num=1024)
y = np.sin(x)
z = np.zeros(len(x))

a, b = np.fft.fft(y)
print('real part:\t', a)
print('\nimaginary part:\t', b)

c, d = np.fft.fft(x, z)
print('\nreal part:\t', c)
print('\nimaginary part:\t', d)

index = 0
while index <= 1024:
    time.sleep(0.05)
    print((a[index],b[index],y[index]*100))
    index = index + 1
