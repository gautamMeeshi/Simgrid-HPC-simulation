'''
have to generate the training data given random scenarios

Generate random scenarios

Find the output string for each random scenario

by sending the value to the API

train a neural network with that data

how many scenarios - 1M

'''

import random

total_nodes = 150

def convertJobListToInputStr(free_nodes,jobs):
    input_bits = [0 for i in range(total_nodes)]
    free_node_list = []
    for i in range(0,free_nodes):
        x = random.randint(0,total_nodes-1)
        while x in free_node_list:
            x = random.randint(0, total_nodes-1)
        free_node_list.append(x)
        input_bits[x] = 1
    for j in jobs:
        input_bits.append(j[0]/total_nodes)
        input_bits.append(j[3]/700)
    while(len(input_bits) < 278):
        input_bits.append(0)
    return input_bits

def generateOutput(free_nodes, jobs):
    output = [0 for i in range(64)]
    for j in range(len(jobs)):
        if jobs[j][0] < free_nodes:
            output[j] = 1
            free_nodes -= jobs[j][0]
    return output

def generateData(njobs):
    input_strings = []
    output_strings = []
    for i in range(100000):
        jobs = []
        free_nodes = random.randint(0,total_nodes)
        rjobs = random.randint(0,njobs)
        for j in range(rjobs):
            nodes = random.randint(1,50)
            tpn = random.randint(1,18)
            cpt = random.randint(1,4)
            comp = random.randint(500,700)
            jobs.append((nodes, tpn, cpt, comp))
        output_strings.append(generateOutput(free_nodes, jobs))
        input_strings.append(convertJobListToInputStr(free_nodes, jobs))
    return input_strings, output_strings

INPUT, OUTPUT = generateData(64)

assert(len(OUTPUT) == len(INPUT))

tf = open('training_data.csv','w+')
tf.write("input,output\n")
for i in range(len(OUTPUT)):
    tf.write(str(INPUT[i]))
    tf.write(',')
    tf.write(str(OUTPUT[i]))
    tf.write('\n')
