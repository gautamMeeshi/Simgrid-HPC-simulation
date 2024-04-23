import socket
import json

def fifo_scheduler(json_data):
    '''
    Input - json data
        {
        'free_nodes': bit_string,
        'jobs': [ [j, nn, tpn, cpt, [job_id1, job_id2,..]], ...]
        }
    Output - bit string of length number of jobs 1 denoting run
    '''
    # print(json_data)
    num_free_nodes = 0
    for i in json_data['free_nodes']:
        if i=='1':
            num_free_nodes+=1
    jobs = json_data['jobs']
    output = ''
    for i in range(len(jobs)):
        if (int(jobs[i][1]) < num_free_nodes):
            output += '1'
            num_free_nodes -= int(jobs[i][1])
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
    res = fifo_scheduler(json.loads(req))
    clientSocket.send(res.encode())

clientSocket.close()
