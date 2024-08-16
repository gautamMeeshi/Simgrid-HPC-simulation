import subprocess
import os
import time

class Stats:
    def __init__(self):
        return
    def __str__(self):
        res = []
        for k,v in self.__dict__.items():
            res.append(f'{k} :  {v}')
        return '\n'.join(res)

def killPython3Processes():
    ret = subprocess.run(['lsof', '-i'], check=True, capture_output=True, text=True)
    stdout = ret.stdout.split('\n')
    stdout = list(map(lambda x: x.split(), ret.stdout.split('\n')))
    pid = -1
    for row in stdout:
        if (len(row) > 2):
            if row[0] == 'python3':
                pid = row[1]
    if pid != -1:
        print(f'killing {pid}')
        ret = subprocess.run(['kill','-9', pid], check=True, capture_output=True, text=True)
        return (ret.returncode == 0)
    else:
        return True

def extractStats(stdout):
    res = Stats()
    # find line containing the string 'Total energy consumed'
    stdout = stdout.split('\n')
    temp = list(filter( lambda x: 'Total energy consumed' in x, stdout))
    assert(len(temp) == 1)
    res.energy = float(temp[0].split()[-1][:-2])
    # find line containing the string 'runtime'
    temp = list(filter( lambda x: 'Total execution time' in x, stdout))
    res.runtime = float(temp[0].split()[-1][:-1])
    temp = list(filter( lambda x: 'EDP of the system' in x, stdout))
    res.edp = float(temp[0].split()[-1][:-3])
    return res

def run100():
    for i in range(85,501):
        try:
            attempts = 8
            print('-'*10, f'Running jobs{i}.csv','-'*10)
            while attempts > 0:
                killPython3Processes()
                time.sleep(5)
                try:
                    print(f'Running bf')
                    result = subprocess.run(['make', 'run', 'SCHED=remote_aggressive_bf', f'JOB_FILE=jobs{i}.csv'],
                                            check=True, capture_output=True, text=True)
                    fcfs_bf_stats = extractStats(result.stderr)
                    print(fcfs_bf_stats)
                    subprocess.run(['mv', './output/run_log.csv', f'./improvements/run_log{i}.csv'])
                    attempts = 0
                except:
                    print("retrying bf")
                    attempts -= 1
            # print(f'Running remote_qnn')
            # attempts = 8
            # rqnn_stats = None
            # while attempts>0:
            #     try:
            #         result = subprocess.run(['make', 'run', 'SCHED=remote_qnn', f'JOB_FILE=jobs{i}.csv'],
            #                                 check=True, capture_output=True, text=True)
            #         rqnn_stats = extractStats(result.stderr)
            #         print(rqnn_stats)
            #         if fcfs_bf_stats.edp > rqnn_stats.edp:
            #             attempts = 0
            #         else:
            #             time.sleep(4)
            #             attempts -= 2
            #     except Exception as e:
            #         print(e)
            #         print('remote_qnn retrying')
            #         time.sleep(8)
            #         attempts -= 1
            # if (rqnn_stats != None and fcfs_bf_stats.edp > rqnn_stats.edp):
            #     print(f'Moving the improved run_log.csv to ./improvements/run_log{i}.csv')
            #     subprocess.run(['mv', './output/run_log.csv', f'./improvements/run_log{i}.csv'])
        except subprocess.CalledProcessError as e:
            print(f"Command {e.cmd} failed with return code:", e.returncode)
            print("Error output:", e.stderr)

if __name__ == "__main__":
    run100()
