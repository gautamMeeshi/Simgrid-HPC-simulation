import socket
import json
import tensorflow as tf
import numpy as np
import random

SCHEDULER_TYPE = None
PORT = 8080
ADDR = "127.0.0.1"
model = None
model7 = None

def LoadModel():
    global model
    model = tf.keras.Sequential([
        # Input layer: 556 features (Node and job information)
        tf.keras.layers.Dense(512, activation='relu', input_shape=(556,)),

        # First hidden layer: 256 neurons
        tf.keras.layers.Dense(256, activation='relu'),

        # Second hidden layer: 128 neurons
        tf.keras.layers.Dense(128, activation='relu'),

        # Output layer: 64 neurons (1 output for each job, whether to run or not)
        tf.keras.layers.Dense(64, activation='sigmoid')
    ])
    model.load_weights('./output/model6.2.keras')


def LoadModel7():
    global model7
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
    # model7.compile(optimizer='adam', 
    #                loss='binary_crossentropy', 
    #                metrics=['accuracy'])
    model7.load_weights('./models/model7.1.keras')

def neural_network_scheduler(json_data):
    global model7
    X =list(map(float, json_data["nn_input"].split(',')))
    X1 = np.array([X[:300]])
    X2 = np.array([X[300:]])
    output = model7.predict([X1, X2], verbose = None)
    output = ','.join([str(output[0][i]) for i in range(64)])
    return (output)

def qnn3(json_data, alpha=0.1):
    '''
    90% schedule according to the neural network
    10% take random step

    record the steps taken, with the flag - nn, random

    store the output for multiple runs

    select the ones that decrease the energy utilization, train on them
    '''
    num_free_nodes = json_data['free_nodes'].count('1')
    job_list = json.loads(json_data['jobs'])
    curr_time = float(json_data['curr_time'])
    json_data['relinquish_times'] = json.loads(json_data['relinquish_times'])
    X = GetNN3Input(json_data['free_nodes'], json_data['relinquish_times'], curr_time, job_list)
    output = ''
    if (random.random() < alpha):
        # take random step
        print("PYTHON INFO: taking random step")
        for i in range(len(job_list)):
            output += str(random.randint(0,1))
    else:
        # generate neural network output
        output = GetNN3Output(num_free_nodes, job_list, X)
    writeRunLog(list(X[0]), output)
    return ('run' + output)

try:
    skt = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
except Exception as e:
    print(e)
    print('PYTHON ERR: socket creation failed')
skt.bind((ADDR, PORT))
skt.listen(5)
print("PYTHON INFO: socket is listening")

clientSocket, addr = skt.accept()

while True:
    req = clientSocket.recv(2**10*100).decode()
    try:
        json_data = json.loads(req)
        # print(json_data)
    except Exception as err:
        print("Failed to load json")
        print(err)
        print(req)
        raise Exception
    operation = json_data['operation']
    if json_data['state'] == 'off':
        print("PYTHON INFO: sensing remote off, stop listening")
        break
    if operation == "init": # in the first iteration SCHEDULER_TYPE is None
        SCHEDULER_TYPE = json_data['scheduler_type']
        print("PYTHON INFO: SCHEDULER TYPE ", SCHEDULER_TYPE)
        if (SCHEDULER_TYPE == 'remote_nn'):
            LoadModel7()
            res = 'ack'
    elif operation == "schedule":
        if(SCHEDULER_TYPE == "remote_nn"):
            res = neural_network_scheduler(json_data)
        else:
            print(f"PYTHON ERR: UNKNOWN SCHEDULER TYPE {SCHEDULER_TYPE}")
            break
        if res:
            clientSocket.send(res.encode())
        else:
            break

print("PYTHON INFO: Closing socket")
clientSocket.close()
