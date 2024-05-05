import numpy as np
from sklearn.model_selection import train_test_split
import tensorflow as tf

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

X,Y = loadTrainingData('training_data.csv')
X_train, X_test, Y_train, Y_test = train_test_split(X, Y, test_size=0.2, random_state=42)

# Create a sequential model
model = tf.keras.Sequential([
    tf.keras.layers.Dense(128, activation='relu', input_shape=X_train.shape[1:]),
    tf.keras.layers.Dense(64, activation='relu'),
    tf.keras.layers.Dense(Y_train.shape[1], activation='sigmoid')  # No activation function for final layer for regression
])

# Compile the model
model.compile(optimizer='adam', loss='mse')

# Print model summary
model.summary()
try:
    model.load_weights("model.weights.h5")
    print("Model weights found, loading...")
except:
    print("Model weights not found, training...")
    model.fit(X_train, Y_train, epochs=40, batch_size=32, verbose=2, validation_data=(X_test, Y_test))
    model.save_weights("model.weights.h5")

Y = model.predict(X_test[:1])
for j in range(Y.shape[0]):
    for i in range(Y.shape[1]):
        Y[j][i] = 1 if Y[j][i] > 0.5 else 0
    print("Prediction:", Y[j])
    print("True val:", Y_test[j])

loss = model.evaluate(X_test, Y_test, verbose=2)
print(f'Test loss: {loss:.4f}')