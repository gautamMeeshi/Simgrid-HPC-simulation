import tarfile
import os
import pickle
import numpy as np
import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers

def extract_tar(tar_filepath, extract_dir="."):
    """
    Extracts a tar archive.

    Args:
        tar_filepath: Path to the tar archive file.
        extract_dir: Directory where the files should be extracted. Defaults to the current directory.
    """
    with tarfile.open(tar_filepath, 'r:gz') as tar:  # 'r:gz' for .tar.gz, 'r' for .tar, 'r:bz2' for .tar.bz2
        tar.extractall(path=extract_dir)  # Extract all members
    print(f"Successfully extracted {tar_filepath} to {extract_dir}")

def load_dataset(extract_dir='.'):
    targets = pickle.load(open(f'{extract_dir}/targets.pkl', 'rb'))
    job_inputs = pickle.load(open(f'{extract_dir}/job_inputs.pkl', 'rb'))
    node_inputs = pickle.load(open(f'{extract_dir}/node_inputs.pkl', 'rb'))

    dataset = tf.data.Dataset.from_tensor_slices(({"job_inputs": job_inputs, "node_inputs": node_inputs}, targets))
    # Shuffle, Batch, and Prefetch
    print(targets.shape)
    num_samples = targets.shape[0]
    batch_size = 16
    train_size = int(0.8 * num_samples)  # 80% Train, 20% Validation
    train_dataset = dataset.take(train_size).shuffle(10000).batch(batch_size).prefetch(tf.data.AUTOTUNE)
    val_dataset = dataset.skip(train_size).batch(batch_size).prefetch(tf.data.AUTOTUNE)
    return train_dataset, val_dataset

tar_file = "data.tar.gz"  # Replace with the actual path to your tar file
extraction_directory = "extracted_files" # Replace with the directory where you want the files to be extracted. If it doesn't exist, create it.

# Create the extraction directory if it doesn't exist
if not os.path.exists(extraction_directory):
    os.makedirs(extraction_directory)

extract_tar(tar_file, extraction_directory)
train_dataset, val_dataset = load_dataset(extraction_directory)
