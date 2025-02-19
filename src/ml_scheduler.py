import socket
import json
import numpy as np
import random
from tensorflow.keras.models import load_model
import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers

SCHEDULER_TYPE = None
PORT = 8080
ADDR = "127.0.0.1"
model = None
model7 = None
transformer_model = None


# === Transformer Encoder Block (with unique naming) ===
def transformer_encoder(embed_dim, num_heads, ff_dim, dropout_rate=0.1, name="TransformerEncoder"):
    inputs = layers.Input(shape=(None, embed_dim))
    x = layers.LayerNormalization()(inputs)
    x = layers.MultiHeadAttention(num_heads=num_heads, key_dim=embed_dim)(x, x)
    x = layers.Dropout(dropout_rate)(x)
    x = layers.Add()([inputs, x])

    x = layers.LayerNormalization()(x)
    x_ffn = layers.Dense(ff_dim, activation="relu")(x)
    x_ffn = layers.Dense(embed_dim)(x_ffn)
    x = layers.Dropout(dropout_rate)(x_ffn)
    x = layers.Add()([x, x_ffn])

    return keras.Model(inputs, x, name=name)

# === Cross-Attention Layer ===
def cross_attention_layer(embed_dim, num_heads):
    query = layers.Input(shape=(None, embed_dim))  # Job embeddings
    key_value = layers.Input(shape=(None, embed_dim))  # Node embeddings
    attn_output = layers.MultiHeadAttention(num_heads=num_heads, key_dim=embed_dim)(query, key_value, key_value)
    return keras.Model([query, key_value], attn_output, name="CrossAttention")

# === Full Transformer Model ===
def build_scheduler_transformer(job_dim=4, node_dim=2, embed_dim=128, num_heads=8, num_layers=4, ff_dim=256):
    # === Inputs ===
    job_inputs = layers.Input(shape=(64, job_dim), name='job_inputs')  # 64 Jobs × 4 Features
    node_inputs = layers.Input(shape=(150, node_dim), name='node_inputs')  # 150 Nodes × 2 Features

    # === Linear Projection ===
    job_proj = layers.Dense(embed_dim)(job_inputs)
    node_proj = layers.Dense(embed_dim)(node_inputs)

    # === Transformer Encoders (with unique names) ===
    job_encoder = transformer_encoder(embed_dim, num_heads, ff_dim, name="JobEncoder")(job_proj)
    node_encoder = transformer_encoder(embed_dim, num_heads, ff_dim, name="NodeEncoder")(node_proj)

    # === Cross-Attention (Jobs query Nodes) ===
    cross_attn = cross_attention_layer(embed_dim, num_heads)([job_encoder, node_encoder])

    # === Output Layer ===
    logits = layers.Dense(1, activation="sigmoid")(cross_attn)  # 1 output per job
    logits = layers.Reshape((64,))(logits)  # Reshape output to (batch_size, 64)

    # === Model ===
    model = keras.Model(inputs=[job_inputs, node_inputs], outputs=logits, name="JobSchedulerTransformer")
    return model

def loadTrainingData(filepath):
    print("loading file", filepath)
    file = open(filepath, 'r')
    header = True
    X1=[]
    X2=[]
    Y=[]
    for line in file:
        s = line[:-1].split('|')
        x = list(map(float, s[0].split(',')))
        y = list(map(float, list(s[1])))
        node_info = []
        for i in range(0,150):
            node_info.append((x[2*i], x[2*i+1]))
        X1.append(node_info)
        job_info = []
        for i in range(0,64):
            job_info.append((x[300+4*i], x[300+4*i+1], x[300+4*i+2], x[300+4*i+3]))
        X2.append(job_info)
        Y.append(y)
    return (X1, X2, Y)

def load_train_val_data():
    '''
    Reads all the improvements and creates training and validation dataset
    '''
    job_inputs = []
    node_inputs = []
    targets = []
    files_to_process = []
    for filename in os.listdir('./improvements'):
        if ('run_log' in filename):
            files_to_process.append(f'{os.getcwd()}/improvements/{filename}')
            # X_t1, X_t2, Y_t = loadTrainingData(f'{ps.getcwd()}/improvements/{filename}')
            # node_inputs.extend(X_t1)
            # job_inputs.extend(X_t2)
            # targets.extend(Y_t)
    # FOLLOWING IS AN EFFORT TO READ THE FILES PARALLELLY - EXPERIMENTAL
    print(f"num_cpus = {multiprocessing.cpu_count()}")
    with multiprocessing.Pool(processes=multiprocessing.cpu_count()) as pool:  # Use all available cores
        results = pool.map(loadTrainingData, files_to_process)
    for X_t1, X_t2, Y_t in results:
        node_inputs.extend(X_t1)
        job_inputs.extend(X_t2)
        targets.extend(Y_t)    
    
    num_samples = len(targets)
    print("num_samples =", num_samples)
    job_inputs = np.array(job_inputs).astype(np.float32)
    node_inputs = np.array(node_inputs).astype(np.float32)
    targets = np.array(targets).astype(np.float32)
    dataset = tf.data.Dataset.from_tensor_slices(({"job_inputs": job_inputs, "node_inputs": node_inputs}, targets))
    # Shuffle, Batch, and Prefetch
    batch_size = 16
    train_size = int(0.8 * num_samples)  # 80% Train, 20% Validation
    train_dataset = dataset.take(train_size).shuffle(10000).batch(batch_size).prefetch(tf.data.AUTOTUNE)
    val_dataset = dataset.skip(train_size).batch(batch_size).prefetch(tf.data.AUTOTUNE)
    return train_dataset, val_dataset

def pairwise_ranking_loss(y_true, y_pred):
    margin = 0.1  # Small margin for ranking
    return tf.reduce_mean(tf.nn.relu(margin - (y_pred - y_true)))

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

def load_transformer_model():
    # === Initialize Model ===
    global transformer_model
    transformer_model = build_scheduler_transformer(
        embed_dim=64,  # Reduce from 128
        num_heads=2,   # Reduce from 8
        num_layers=2,    # Reduce from 4
        ff_dim=128
    )
    transformer_model.load_weights("./models/scheduler_transformer_model_sgd1.keras")
    transformer_model.summary()
    return

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

def transformer_scheduler(json_data):
    global transformer_model
    X = list(map(float, json_data["nn_input"].split(',')))
    node_data = np.array(X[:300]).reshape(1, 150, 2)
    job_data = np.array(X[300:]).reshape(1, 64, 4)
    transformer_input = {"job_inputs": job_data, "node_inputs": node_data}
    output = list(map(str, transformer_model.predict(transformer_input, verbose = None)[0].tolist()))
    output = ','.join(output)
    return output

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
        elif (SCHEDULER_TYPE == 'remote_transformer'):
            load_transformer_model()
            res = 'ack'
    elif operation == "schedule":
        if (SCHEDULER_TYPE == "remote_nn"):
            res = neural_network_scheduler(json_data)
        elif (SCHEDULER_TYPE == "remote_transformer"):
            res = transformer_scheduler(json_data)
        else:
            print(f"PYTHON ERR: UNKNOWN SCHEDULER TYPE {SCHEDULER_TYPE}")
            break
        if res:
            clientSocket.send(res.encode())
        else:
            break

print("PYTHON INFO: Closing socket")
clientSocket.close()
