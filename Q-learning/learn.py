import numpy as np
import tensorflow as tf
import os

def loadTrainingData(filepath):
    file = open(filepath, 'r')
    header = True
    X,Y = [],[]
    for line in file:
        if header:
            header = False
            continue
        line = line.split('],[')
        X.append(list(map(float, line[0][1:].split(','))))
        try:
            Y.append(list(map(float, line[1][:-2].split(','))))
        except:
            print(line[1])
            raise Exception
    X = np.array(X)
    Y = np.array(Y)
    return X,Y

# Create a sequential model
#model3 = tf.keras.Sequential([
#        tf.keras.layers.Dense(171, activation='relu', input_shape=(342,)),
#        tf.keras.layers.Dense(128, activation='relu'),
#        tf.keras.layers.Dense(64, activation='sigmoid')
#])

# model4 = tf.keras.Sequential([
#     tf.keras.layers.Dense(171, activation='tanh', input_shape=(342,)),
#     tf.keras.layers.Dense(128, activation='tanh'),
#     tf.keras.layers.Dense(64, activation='sigmoid')
# ])

# input - 150*2 + 64*4
model = tf.keras.Sequential([
    tf.keras.layers.Dense(278, activation='tanh', input_shape=(556,)),
    tf.keras.layers.Dense(139, activation='tanh'),
    tf.keras.layers.Dense(64, activation='sigmoid')
])


# Compile the model
model.compile(optimizer='adam', loss='mse')

# Print model summary
model.summary()
model.load_weights("./models/qmodel5.5.weights.h5")
print("Model weights found, training...")

# Read the files in improvement/ dir and add
for filename in os.listdir('./improvements'):
    if ('run_log' in filename):
        print(f'training on ./improvements/{filename}')
        X,Y = loadTrainingData('./improvements/'+filename)
        model.fit(X, Y, epochs=10, batch_size=32, verbose=2)

model.save_weights("./models/qmodel5.6.weights.h5")
