import socket
import json
import tensorflow as tf
import numpy as np

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
        for i in range(5):
            job.append(j[i])
        if j[5] == "":
            job.append([])
        else:
            pids = []
            for p in j[5]:
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
        if (jobs[jid][1] == 0 and allParentsCompleted(jobs, jobs[jid][5])):
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
        Pc = 1/jobs[jid][4]**0.5 # give more priority to the jobs that have lesser computation
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
        'jobs': [ [j, job_state, nn, tpn, cpt, [job_id1, job_id2,..]], ...]
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
            print('RUNNING ', runnable_jobs[i][0])
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
model.summary()
try:
    model.load_weights("./utils/model.weights.h5")
    print("Model weights found, loading...")
except Exception as e:
    print(e)

def neural_network_scheduler(json_data):
    global model
    output = 'run'
    X = []
    for i in json_data['free_nodes']:
        if i == '1':
            X.append(1)
        else:
            X.append(0)
    #print(json_data['jobs'])
    job_list = json.loads(json_data['jobs'])
    for i in range(64):
        if i < len(job_list):
            X.append(job_list[i][1]/150)
            X.append(job_list[i][4]/700*10**7)
        else:
            X.append(0)
            X.append(0)
    X = np.array([X])
    Y = model.predict(X)
    for i in range(len(job_list)):
        if (Y[0][i] == 1):
            output += '1'
        else:
            output += '0'
    return output

PORT = 8080
ADDR = "127.0.0.1"
skt = None

try:
    skt = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
except Exception as e:
    print(e)
    print('socket creation failed')
skt.bind((ADDR, PORT))
skt.listen(5)
print("socket is listening")

clientSocket, addr = skt.accept()

while True:
    # try:
    req = clientSocket.recv(2**10*10).decode()
    res = neural_network_scheduler(json.loads(req))
    clientSocket.send(res.encode())
    # except Exception as e:
    #     print(e)
    #     break

clientSocket.close()
