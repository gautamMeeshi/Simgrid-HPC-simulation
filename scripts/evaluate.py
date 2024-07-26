import subprocess
import time

class Stats:
    def __init__(self):
        return
    def __str__(self):
        res = []
        for k,v in self.__dict__.items():
            res.append(f'{k} :  {v}')
        return '\n'.join(res)

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
    log_file = open('log.csv', 'a')
    log_file.write('job_name,E_fcfs_bf,T_fcfs_bf,EDP_fcfs_bf,E_nn,T_nn,EDP_nn,E_imp,T_imp,EDP_imp\n')
    total_jobs_run = 0
    njobs_with_edp_improvement = 0
    njobs_with_E_improvement = 0
    njobs_with_T_improvement = 0
    for i in range(101,201):
        try:
            print('-'*10, f'Running jobs{i}.csv','-'*10)
            print(f'Running fcfs_bf')
            log_file.write(f'job{i},')
            result = subprocess.run(['make', 'run', 'SCHED=fcfs_backfill', f'JOB_FILE=jobs{i}.csv'],
                                    check=True, capture_output=True, text=True)
            fcfs_bf_stats = extractStats(result.stderr)
            print(fcfs_bf_stats)
            print(f'Running remote_nn')
            attempts = 8
            nn_stats = None
            while attempts>0:
                try:
                    result = subprocess.run(['make', 'run', 'SCHED=remote_nn', f'JOB_FILE=jobs{i}.csv'],
                                            check=True, capture_output=True, text=True)
                    nn_stats = extractStats(result.stderr)
                    attempts = 0
                except:
                    print("Remote nn failed, retrying")
                    attempts -=1
                    time.sleep(5)
            if nn_stats == None:
                continue
            total_jobs_run +=1
            print(nn_stats)
            log_file.write(f'{fcfs_bf_stats.energy},{fcfs_bf_stats.runtime},{fcfs_bf_stats.edp},')
            log_file.write(f'{nn_stats.energy},{nn_stats.runtime},{nn_stats.edp},')
            energy_improvement = (fcfs_bf_stats.energy-nn_stats.energy)/fcfs_bf_stats.energy*100
            runtime_improvement = (fcfs_bf_stats.runtime-nn_stats.runtime)/fcfs_bf_stats.runtime*100
            edp_improvement = (fcfs_bf_stats.edp-nn_stats.edp)/fcfs_bf_stats.edp*100
            if energy_improvement > 0:
                njobs_with_E_improvement += 1
            if runtime_improvement > 0:
                njobs_with_T_improvement += 1
            if edp_improvement > 0:
                njobs_with_edp_improvement += 1
            log_file.write(f'''{energy_improvement},{runtime_improvement},{edp_improvement}\n''')
        except subprocess.CalledProcessError as e:
            print(f"Command {e.cmd} failed with return code:", e.returncode)
            print("Error output:", e.stderr)
    log_file.write(f'Total number of jobs run sucessfully = {total_jobs_run}\n')
    log_file.write(f'No. jobs with energy improvement = {njobs_with_E_improvement}\n')
    log_file.write(f'No. jobs with runtime improvement = {njobs_with_T_improvement}\n')
    log_file.write(f'No. jobs with edp improvement = {njobs_with_edp_improvement}\n')
    log_file.close()
if __name__ == "__main__":
    run100()
