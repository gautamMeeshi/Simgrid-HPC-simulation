import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers
import numpy as np
import os
import multiprocessing
tf.keras.mixed_precision.set_global_policy("mixed_float16")
import pickle

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

    
    pickle.dump(targets, open('targets.pkl', 'wb'))
    del targets
    pickle.dump(job_inputs, open('job_inputs.pkl', 'wb'))
    del job_inputs
    pickle.dump(node_inputs, open('node_inputs.pkl', 'wb'))
    del node_inputs
    # dataset = tf.data.Dataset.from_tensor_slices(({"job_inputs": job_inputs, "node_inputs": node_inputs}, targets))
    # # Shuffle, Batch, and Prefetch
    # batch_size = 16
    # train_size = int(0.8 * num_samples)  # 80% Train, 20% Validation
    # train_dataset = dataset.take(train_size).shuffle(10000).batch(batch_size).prefetch(tf.data.AUTOTUNE)
    # val_dataset = dataset.skip(train_size).batch(batch_size).prefetch(tf.data.AUTOTUNE)
    # return train_dataset, val_dataset


load_train_val_data()
