import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers
import numpy as np
import os
import multiprocessing
tf.keras.mixed_precision.set_global_policy("mixed_float16")
tf.config.optimizer.set_jit(True)

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

# === Initialize Model ===
model = build_scheduler_transformer(
    embed_dim=64,  # Reduce from 128
    num_heads=2,   # Reduce from 8
    num_layers=2,    # Reduce from 4
    ff_dim=128
)
model.summary()


def pairwise_ranking_loss(y_true, y_pred):
    margin = 0.1  # Small margin for ranking
    return tf.reduce_mean(tf.nn.relu(margin - (y_pred - y_true)))

# loss_fn = keras.losses.BinaryCrossentropy()
loss_fn = pairwise_ranking_loss

# optimizer = keras.optimizers.AdamW(learning_rate=1e-4)
optimizer = keras.optimizers.SGD(learning_rate=1e-3, momentum=0.9)

# Compile Model
model.compile(optimizer=optimizer, loss=loss_fn)

train_dataset, val_dataset = load_train_val_data()

# Train the model
history = model.fit(
    train_dataset,  # (job_inputs, node_inputs), targets
    epochs=1,
    validation_data=val_dataset,
    verbose=2
)

model.save("scheduler_transformer_model_sgd1.keras")
