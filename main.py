
from scipy.stats import pearsonr
import numpy as np
from math import sqrt
from graph import plot_data_and_fit

x_data=[2,5,7,10,14,18]
y_data=[10.9,7.5,3.3,5.7,4.3,2.6]

x_array = np.array(x_data)
y_array = np.array(y_data)

Y1_array = 1 / y_array
X1_array = np.exp(-x_array)

corrolation_factor,_=pearsonr(X1_array,Y1_array)


popt=np.polyfit(X1_array,Y1_array,1)
a=popt[1]
b=popt[0]


y_hat = 1 / (a + b * np.exp(-x_array))
y_bar=np.mean(y_array)

sr= np.sum((y_array - y_hat) ** 2)
st= np.sum((y_array - y_bar) ** 2)
r=sqrt((st - sr) / st)
print(f"a={a},b={b},sr={sr},st={st},r={r}")

plot_data_and_fit(x_data, y_data, x_array, y_hat)