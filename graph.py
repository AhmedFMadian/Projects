import matplotlib.pyplot as plt
def plot_data_and_fit(x_data, y_data, x_fit, y_fit):
    plt.scatter(x_data, y_data, color='blue', label='Data Points')
    plt.plot(x_fit, y_fit, color='red', label='Fitted Curve')
    plt.xlabel('X-axis')
    plt.ylabel('Y-axis')
    plt.title('Data and Fitted Curve')
    plt.legend()
    plt.grid(True)
    plt.show()