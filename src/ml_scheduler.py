import socket
import json
import tensorflow as tf
import numpy as np
import random

SCHEDULER_TYPE = None
PORT = 8080
ADDR = "127.0.0.1"

try:
    skt = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
except Exception as e:
    print(e)
    print('PYTHON ERR: socket creation failed')
skt.bind((ADDR, PORT))
skt.listen(5)
print("PYTHON INFO: socket is listening")

clientSocket, addr = skt.accept()

def LoadModel4():
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
    model = tf.keras.Model(inputs=[node_input, job_input], outputs=output)
    # Compile the model
    model.compile(optimizer='adam', 
                  loss='binary_crossentropy', 
                  metrics=['accuracy'])
    # Model summary to see the layers and parameters
    model.summary()

def LoadModel3():
    '''
    Input - 150*2 (free node, (relinquish_time-curr_time)), 64*4 (computation,nodes,cum_runtime,runtime)
    '''
    global model3
    model3 = tf.keras.Sequential([
        tf.keras.layers.Dense(278, activation='tanh', input_shape=(556,)),
        tf.keras.layers.Dense(139, activation='tanh'),
        tf.keras.layers.Dense(64, activation='sigmoid')
    ])
    model3.compile(optimizer='adam', loss='mse')
    model3.summary()
    try:
        model3.load_weights("./models/qmodel5.6.weights.h5")
        print("PYTHON INFO: Model weights found, loading...")
    except Exception as e:
        print(e)
    return

def neural_network_scheduler(json_data):
    X = np.array([list(map(float, json_data["nn_input"].split(',')))])
    assert(X.shape == (1,556))
    output = model3.predict(X, verbose = None)
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
            LoadModel3()
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
