Dimensions

model1
    model = tf.keras.Sequential([
        tf.keras.layers.Dense(128, activation='relu', input_shape=(278,)),
        tf.keras.layers.Dense(64, activation='relu'),
        tf.keras.layers.Dense(64, activation='sigmoid')  # No activation function for final layer for regression
    ])

model2
    model2 = tf.keras.Sequential([
        tf.keras.layers.Dense(128, activation='relu', input_shape=(342,)),
        tf.keras.layers.Dense(64, activation='relu'),
        tf.keras.layers.Dense(64, activation='sigmoid')  # No activation function for final layer for regression
    ])

model3
    model3 = tf.keras.Sequential([
        tf.keras.layers.Dense(171, activation='relu', input_shape=(342,)),
        tf.keras.layers.Dense(128, activation='relu'),
        tf.keras.layers.Dense(64, activation='sigmoid')  # No activation function for final layer for regression
    ])

model4
    model2 = tf.keras.Sequential([
        tf.keras.layers.Dense(171, activation='tanh', input_shape=(342,)),
        tf.keras.layers.Dense(128, activation='tanh'),
        tf.keras.layers.Dense(64, activation='sigmoid')  # No activation function for final layer for regression
    ])