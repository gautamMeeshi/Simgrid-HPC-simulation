import numpy as np
import tensorflow as tf
import os
from sklearn.model_selection import train_test_split

def loadTrainingData(filepath):
    file = open(filepath, 'r')
    header = True
    X1=[]
    X2=[]
    Y=[]
    for line in file:
        x = line[:-1].split(',')
        y = x[-1][-64:]
        x[-1] = x[-1][:-64]
        x = list(map(float, x))
        y = list(map(float, list(y)))
        X1.append(x[:300])
        X2.append(x[300:])
        Y.append(y)
    return (X1, X2, Y)
    # Y = np.array(Y)
    # return X,Y

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
# model5 = tf.keras.Sequential([
#     tf.keras.layers.Dense(278, activation='tanh', input_shape=(556,)),
#     tf.keras.layers.Dense(139, activation='tanh'),
#     tf.keras.layers.Dense(64, activation='sigmoid')
# ])


# Input 1: Node information (300 floats)
node_input = tf.keras.layers.Input(shape=(300,))
node_dense_1 = tf.keras.layers.Dense(128, activation='relu')(node_input)
node_dense_2 = tf.keras.layers.Dense(64, activation='relu')(node_dense_1)

# Input 2: Job information (256 floats)
job_input = tf.keras.layers.Input(shape=(256,))
job_dense_1 = tf.keras.layers.Dense(128, activation='relu')(job_input)
job_dense_2 = tf.keras.layers.Dense(64, activation='relu')(job_dense_1)

# Concatenate the outputs from both paths
concat = tf.keras.layers.Concatenate()([node_dense_2, job_dense_2])

# Process the combined features
combined_dense_1 = tf.keras.layers.Dense(128, activation='relu')(concat)
combined_dense_2 = tf.keras.layers.Dense(64, activation='relu')(combined_dense_1)

# Output layer: 64 outputs (one for each job, indicating whether it should run or not)
output = tf.keras.layers.Dense(64, activation='sigmoid')(combined_dense_2)

# Define the model with two inputs
model7 = tf.keras.Model(inputs=[node_input, job_input], outputs=output)

# Compile the model
model7.compile(optimizer='adam', 
              loss='binary_crossentropy', 
              metrics=['accuracy'])

# Summary of the model
model7.summary()



# Define the model
model6 = tf.keras.Sequential([
    # Input layer: 556 features (Node and job information)
    tf.keras.layers.Dense(512, activation='relu', input_shape=(556,)),

    # First hidden layer: 256 neurons
    tf.keras.layers.Dense(256, activation='relu'),

    # Second hidden layer: 128 neurons
    tf.keras.layers.Dense(128, activation='relu'),

    # Output layer: 64 neurons (1 output for each job, whether to run or not)
    tf.keras.layers.Dense(64, activation='sigmoid')
])

# Compile the model
model6.compile(optimizer='adam', 
              loss='binary_crossentropy', 
              metrics=['accuracy'])

# Model summary to see the layers and parameters
model6.summary()

# print("Model weights found, training...")

# Read the files in improvement/ dir and add
X1 = []
X2 = []
Y = []
for filename in os.listdir('./improvements'):
    if ('run_log' in filename):
        print(f'loading ./improvements/{filename}')
        X_t1, X_t2, Y_t = loadTrainingData('./improvements/'+filename)
        X1.extend(X_t1)
        X2.extend(X_t2)
        Y.extend(Y_t)

X1 = np.array(X1)
X2 = np.array(X2)
Y = np.array(Y)

X1_train, X1_val, X2_train, X2_val, Y_train, Y_val = train_test_split(X1, X2, Y, test_size=0.1, random_state=42)

checkpoint_callback = tf.keras.callbacks.ModelCheckpoint(
    filepath='./output/model7.1.keras',     # Path where the best model will be saved
    monitor='val_accuracy',       # Monitor validation accuracy
    save_best_only=True,          # Save only the model with the best accuracy
    mode='max',                   # 'max' since we want to maximize accuracy
    verbose=1
)

# model.load_weights('./output/model6.2.keras')

model7.fit([X1_train, X2_train], Y_train, epochs=30, batch_size=64, verbose=2,
           validation_data=([X1_val, X2_val], Y_val), callbacks = [checkpoint_callback])

# model7.save_weights("./models/qmodel7.1.weights.h5")
