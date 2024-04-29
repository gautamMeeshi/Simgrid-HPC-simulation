import socket
import json

def parseJobsJson(jobs_json):
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
    for p in p_job_ids:
        if (jobs[p][1] != 2):
            return False
    return True

def getRunnableJobs(jobs):
    res = []
    for jid in jobs:
        if (jobs[jid][1] == 0 and allParentsCompleted(jobs, jobs[jid][5])):
            res.append(jobs[jid])
    return res

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
    output = ''
    for i in range(len(runnable_jobs)):
        if (runnable_jobs[i][2] <= num_free_nodes):
            # output += str(runnable_jobs[i][0])+','
            output += '1'
            print('RUNNING ', runnable_jobs[i][0])
            num_free_nodes -= runnable_jobs[i][2]
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
    req = clientSocket.recv(2**10*10).decode()
    res = fcfsBackfillScheduler(json.loads(req))
    clientSocket.send(res.encode())

clientSocket.close()
