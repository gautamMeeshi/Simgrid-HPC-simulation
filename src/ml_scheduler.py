import socket
import json
import tensorflow as tf
import numpy as np
import random
import threading

SCHEDULER_TYPE = None
TRAINER_THREAD = threading.Thread()
TRAIN_NN = False
RUN_LOG = open('output/run_log.csv', 'w+')
RUN_LOG.write("input,output\n")
Xs = []
Ys = []

def parseJobsJson(jobs_json):
    '''
    input - list of [[jid, nn, tpn, cpt, comp, [pid1, pid2]], []  . . . .]
    output - dictionary
    key : job_id
    value : list (jid, nn, tpn, cpt, comp, [pid1, pid2, ...])
    '''
    jobs_json = json.loads(jobs_json)
    jobs = {}
    for j in jobs_json:
        job = []
        for i in range(6):
            job.append(j[i])
        if j[5] == "":
            job.append([])
        else:
            pids = []
            for p in j[6]:
                pids.append(int(p))
            job.append(pids)
        jobs[job[0]] = job
    return jobs

def allParentsCompleted(jobs, p_job_ids):
    '''
    input - jobs dictionary, parent jod id list
    output - whether all the jobs with the parent job id have completed?
    '''
    for p in p_job_ids:
        if (jobs[p][1] != 2):
            return False
    return True

def getRunnableJobs(jobs):
    '''
    input - jobs dictionary
    output - the jobs for which all the parents jobs have completed
    '''
    res = []
    for jid in jobs:
        if (jobs[jid][1] == 0 and allParentsCompleted(jobs, jobs[jid][6])):
            res.append(jobs[jid])
    return res

def calculateJobPriorityScores(jobs):
    '''
    returns a dictionary
    key job_id
    value score
    '''
    least_jid = min(jobs.keys())
    Pf = {}
    dependent = {}
    for jid in jobs:
        dependent[jid] = []
        if (jobs[jid][1] == 2):
            continue # skip the score calculation for the jobs that have completed
        Ps = 1/(jid + 1 - least_jid) # give more priority to the jobs according to the queue
        Pc = 1/jobs[jid][5]**0.5 # give more priority to the jobs that have lesser computation
        Pn = 1/jobs[jid][2] # give more priority to the jobs that reserve lesser number of nodes
        Pf[jid] = 0.5*Ps + 0.4*Pc + 0.1*Pn # weighted sum of the factors
        for pid in jobs[jid][5]:
            dependent[pid].append(jid)
    for jid in jobs: # jobs having more dependent jobs 
        if (jobs[jid][1] == 2):
            continue
        for cid in dependent[jid]:
            Pf[jid] += 0.1/(cid-jid)*Pf[cid]
    return Pf

def heuristic_scheduler(json_data):
    '''
    Input - json data
        {
        'free_nodes': bit_string,
        'jobs': [ [j, job_state, nn, tpn, cpt, comp, [job_id1, job_id2,..]], ...]
        }
    Output - string containing the command seperated list of job_ids to run
    '''
    num_free_nodes = json_data['free_nodes'].count('1')
    jobs = parseJobsJson(json_data['jobs'])
    runnable_jobs = getRunnableJobs(jobs)
    rids = [job[0] for job in runnable_jobs]
    job_priority_scores = calculateJobPriorityScores(jobs)
    runnable_jobs.sort(key = lambda x: job_priority_scores[x[0]])
    jobs_to_run = {}
    for i in range(len(runnable_jobs)):
        if (runnable_jobs[i][2] <= num_free_nodes):
            jobs_to_run[runnable_jobs[i][0]] = True
            num_free_nodes -= runnable_jobs[i][2]
        else:
            break
    output = 'run'
    for jid in rids:
        if (jid in jobs_to_run):
            output += '1'
        else:
            output += '0'
    return output

def fcfsBackfillScheduler(json_data):
    '''
    Input - json data
        {
        'free_nodes': bit_string,
        'jobs': [ [jid, job_state, nn, tpn, cpt, [job_id1, job_id2,..]], ...]
        }
    Output - bit string of length number of jobs 1 denoting run
    '''
    num_free_nodes = json_data['free_nodes'].count('1')
    jobs = parseJobsJson(json_data['jobs'])
    runnable_jobs = getRunnableJobs(jobs)
    output = 'run'
    for i in range(len(runnable_jobs)):
        if (runnable_jobs[i][2] <= num_free_nodes):
            # output += str(runnable_jobs[i][0])+','
            output += '1'
            print('PYTHON INFO: RUNNING ', runnable_jobs[i][0])
            num_free_nodes -= runnable_jobs[i][2]
        else:
            output += '0'
    return output

# Create a sequential model
model = tf.keras.Sequential([
    tf.keras.layers.Dense(128, activation='relu', input_shape=(278,)),
    tf.keras.layers.Dense(64, activation='relu'),
    tf.keras.layers.Dense(64, activation='sigmoid')  # No activation function for final layer for regression
])
model.compile(optimizer='adam', loss='mse')
model.summary()
try:
    model.load_weights("./utils/model.weights.h5")
    print("PYTHON INFO: Model weights found, loading...")
except Exception as e:
    print(e)

def train(X, Y):
    global model
    global TRAIN_NN
    print('PYTHON INFO: training with', Y.shape[0], 'data points')
    model.fit(X, Y, epochs=10, batch_size=32, verbose=0)
    model.save_weights("utils/model.weights.h5")

def writeRunLog(free_nodes, inp, out):
    '''
    free_nodes - string denoting the free nodes
    inp - job data
    out - bit string denoting the jobs to be run (64 in len)
    '''
    global RUN_LOG
    X = list(map(int, free_nodes))
    X.extend(inp)
    RUN_LOG.write(str(X)+',')
    Y = list(map(int, out))
    Y.extend([0]*(64-len(Y)))
    RUN_LOG.write(str(Y)+'\n')
    return
        

def neural_network_scheduler(json_data):
    global model
    global RUN_LOG
    num_free_nodes = json_data['free_nodes'].count('1')
    X = []
    for i in json_data['free_nodes']:
        if i == '1':
            X.append(1)
        else:
            X.append(0)
    job_list = json.loads(json_data['jobs'])
    for i in range(64):
        if i < len(job_list):
            X.append(job_list[i][2]/150)
            X.append(job_list[i][5]/(700*10**11))
        else:
            X.append(0)
            X.append(0)
    X = np.array([X])
    Y = model.predict(X, verbose = None)
    output = ['0']*len(job_list)
    for i in range(0, min(64, len(job_list))):
        if (job_list[i][2] <= num_free_nodes and Y[0][i] > 0.5):
            output[i] = '1'
            num_free_nodes -= job_list[i][2]
    for i in range(0, len(job_list)):
        if (job_list[i][2] <= num_free_nodes and output[i] == '0'):
            output[i] = '1'
            num_free_nodes -= job_list[i][2]
    output = ''.join(output)
    writeRunLog(json_data['free_nodes'], list(X[0]), output)
    output = 'run' + output
    return output

INPUT = []
OUTPUT = []

def learning_neural_network(json_data):
    '''
    this function creates data
    runs both neural network and fcfs
    the input and output of fcfs is stored
    At regular intervals the nerual network is trained against the fcfs output

    For now lets completely not use the neural network output
    '''
    global TRAINER_THREAD
    global TRAIN_NN
    global Xs
    global Ys
    output = 'run'
    num_free_nodes = json_data['free_nodes'].count('1')
    job_list = json.loads(json_data['jobs'])
    Xs.append([])
    Ys.append([])
    for i in json_data['free_nodes']:
        if i == '1':
            Xs[-1].append(1)
        else:
            Xs[-1].append(0)
    for i in range(64):
        if i < len(job_list):
            Xs[-1].append(job_list[i][2]/150)
            Xs[-1].append(job_list[i][5]/(700*10**11))
        else:
            Xs[-1].append(0)
            Xs[-1].append(0)
    # follow fcfs_bf
    for i in range(len(job_list)):
        if (job_list[i][2] <= num_free_nodes):
            output += '1'
            Ys[-1].append(1)
            num_free_nodes -= job_list[i][2]
        else:
            Ys[-1].append(0)
            output += '0'
    if len(Ys[-1]) > 64:
        Ys[-1] = Ys[-1][:64]
    else:
        Ys[-1].extend([0]*(64-len(Ys[-1])))
    if (json_data['train'] == 'True'):
        # train the neural network using a different thread
        TRAIN_NN = True

    if (TRAIN_NN and TRAINER_THREAD.is_alive() == False):
        TRAIN_NN = False
        TRAINER_THREAD = threading.Thread(target = train, args = (np.array(Xs), np.array(Ys)))
        TRAINER_THREAD.start()
        Xs = []
        Ys = []
    return output

def qnn(json_data, alpha = 0.1):
    '''
    90% schedule according to the neural network
    10% take random step

    record the steps taken, with the flag - nn, random

    store the output for multiple runs

    select the ones that decrease the energy utilization, train on them
    '''
    global model
    global INPUT
    global OUTPUT
    output = ''
    X = []
    for i in json_data['free_nodes']:
        if i == '1':
            X.append(1)
        else:
            X.append(0)
    job_list = json.loads(json_data['jobs'])
    for i in range(64):
        if i < len(job_list):
            X.append(job_list[i][2]/150)
            X.append(job_list[i][5]/700*10**7)
        else:
            X.append(0)
            X.append(0)
    if (random.random() < alpha):
        # take random step
        print("PYTHON INFO: taking random step")
        INPUT.append(X)
        for i in range(len(job_list)):
            output += str(random.randint(0,1))
        Y = []
        for i in range(64):
            if i < len(job_list):
                Y.append(int(output[i]))
            else:
                Y.append(0)
        OUTPUT.append(Y)
    else:
        # generate neural network output
        X = np.array([X])
        Y = model.predict(X)
        for i in range(len(job_list)):
            if (Y[0][i] > 0.5):
                output += '1'
            else:
                output += '0'
    return ('run'+output)

PORT = 8080
ADDR = "127.0.0.1"
skt = None

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
    req = clientSocket.recv(2**10*10).decode()
    json_data = json.loads(req)
    if json_data['state'] == 'off':
        print("PYTHON INFO: sensing remote off, stop listening")
        break
    if SCHEDULER_TYPE == None:
        SCHEDULER_TYPE = json_data['scheduler_type']
        print("PYTHON INFO: SCHEDULER TYPE ", SCHEDULER_TYPE)
    else:
        if (SCHEDULER_TYPE == "remote_heuristic"):
            res = heuristic_scheduler(json_data)
        elif (SCHEDULER_TYPE == "remote_fcfs_bf"):
            res = fcfsBackfillScheduler(json_data)
        elif (SCHEDULER_TYPE == "remote_learn_neural_network"):
            res = learning_neural_network(json_data)
        elif (SCHEDULER_TYPE == "remote_neural_network"):
            res = neural_network_scheduler(json_data)
        elif (SCHEDULER_TYPE == "remote_qnn"):
            res = qnn(json_data)
        else:
            print(f"PYTHON ERR: UNKNOWN SCHEDULER TYPE {SCHEDULER_TYPE}")
            break
        if res:
            clientSocket.send(res.encode())
        else:
            break


print("PYTHON INFO: Closing socket")
clientSocket.close()
RUN_LOG.close()
