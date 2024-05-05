num_cpus = 2
num_cores = 6
core_speed = '2Gf'
power_consumption = "140.0:320.0"
link_lat = "100us"
link_bw = "100MBps"
loopback_bw = "500MBps"
loopback_lat = "10us"

xdim = 5
ydim = 5
zdim = 6

# Create the node names - vertices
node_names = {}
for x in range(xdim):
    for y in range(ydim):
        for z in range(zdim):
            node_names[(x,y,z)] = f"node-{x}.{y}.{z}"

# Create links - same as edges
link_names = {} # key - two node names it connects, value same as key

# links parallel to x axis
for x in range(xdim):
    for y in range(ydim):
        for z in range(zdim):
            if (x<xdim-1):
                link_names[(f'node-{x}.{y}.{z}', f'node-{x+1}.{y}.{z}')] = f"link-{x}.{y}.{z}-{x+1}.{y}.{z}"
            else:
                link_names[(f'node-{x}.{y}.{z}', f'node-{0}.{y}.{z}')] = f"link-{x}.{y}.{z}-{0}.{y}.{z}"

# link parallel to y axis
for x in range(xdim):
    for y in range(ydim):
        for z in range(zdim):
            if (y<ydim-1):
                link_names[(f'node-{x}.{y}.{z}', f'node-{x}.{y+1}.{z}')] = f"link-{x}.{y}.{z}-{x}.{y+1}.{z}"
            else:
                link_names[(f'node-{x}.{y}.{z}', f'node-{x}.{0}.{z}')] = f"link-{x}.{y}.{z}-{x}.{0}.{z}"

# link parallel to z axis
for x in range(xdim):
    for y in range(ydim):
        for z in range(zdim):
            if (z<zdim-1):
                link_names[(f'node-{x}.{y}.{z}', f'node-{x}.{y}.{z+1}')] = f"link-{x}.{y}.{z}-{x}.{y}.{z+1}"
            else:
                link_names[(f'node-{x}.{y}.{z}', f'node-{x}.{y}.{0}')] = f"link-{x}.{y}.{z}-{x}.{y}.{0}"

# print(link_names.keys())

adj_grf = {}

for l in link_names:
    if l[0] in adj_grf:
        adj_grf[l[0]].append(l[1])
    else:
        adj_grf[l[0]] = [l[1]]
    if l[1] in adj_grf:
        adj_grf[l[1]].append(l[0])
    else:
        adj_grf[l[1]] = [l[0]]
# print(adj_grf.keys())

def Dijkstras(adj_grf, source):
    done = dict([(k, False) for k in adj_grf])
    dist = dict([(k, float('inf')) for k in adj_grf])
    prev = dict([(k, None) for k in adj_grf])
    dist[source] = 0
    
    while True:
        # Find the not done node having the smallest dist
        min_node = None
        min_dist = float('inf')
        for k in done:
            if (not done[k]) and (dist[k] < min_dist):
                min_node = k
                min_dist = dist[k]
        if (min_node == None):
            break
        done[min_node] = True
        for nei in adj_grf[min_node]:
            if (dist[nei] > dist[min_node] + 1):
                dist[nei] = dist[min_node] + 1 # ASSUMPTION all edges are of same length
                prev[nei] = min_node
    return (dist, prev)

def construct_paths(prev, source):
    global node_names
    paths = {}
    for (k,node) in node_names.items():
        path = []
        nodec = node
        while(nodec != None):
            path.append(nodec)
            nodec = prev[nodec]
        path.reverse()
        paths[(source,node)] = path
    return paths

def getAllRoutes():
    global node_names
    global adj_grf
    routes = {} # key pair of nodes, value list of links
    for v in node_names.values():
        (dist,prev) = Dijkstras(adj_grf, v)
        paths = construct_paths(prev, v)
        routes.update(paths)

    nodes = list(node_names.values())
    for i in range(len(nodes)):
        for j in range(i+1,len(nodes)):
            del routes[(nodes[j], nodes[i])]
    return routes

def getRoutesFromSlurmCtlD():
    global node_names
    global adj_grf
    v = node_names[(xdim//2, ydim//2, zdim//2)]
    (dist, prev) = Dijkstras(adj_grf, v)
    return construct_paths(prev, v)

# routes = getAllRoutes()
routes = getRoutesFromSlurmCtlD()

################### CREATE PLATFORM FILE #######################
pf_file = open(f'platform-{xdim}.{ydim}.{zdim}.{num_cpus}-torus.xml','w+')

pf_file.write('''<?xml version='1.0'?>
<!DOCTYPE platform SYSTEM "https://simgrid.org/simgrid.dtd">
<platform version="4.1">
    <config>
        <prop id="tracing" value="yes"/>
        <prop id="tracing/uncategorized" value="yes"/>
    </config>
    <zone id="zone0" routing="Full">
''')
pf_file.write("<!-- CPUS spec -->\n")
for node in node_names.values():
    for c in range(num_cpus):
        pf_file.write(f'''  <host id="{node}.c{c}" speed="{core_speed}" core="{num_cores}">
    <prop id="wattage_per_state" value="{power_consumption}"/>
    <prop id="wattage_off" value="10"/>
  </host>
''')

pf_file.write("<!-- link spec -->\n")
pf_file.write("<!-- inter node links -->\n")
for v in link_names.values():
    pf_file.write(f'''  <link id="{v}" bandwidth="{link_bw}" latency="{link_lat}"/>\n''')

pf_file.write("<!-- intra node, inter core links -->\n")
for x in range(xdim):
    for y in range(ydim):
        for z in range(zdim):
            for c in range(1,num_cpus):
                name = f'link-{x}.{y}.{z}.{0}-{x}.{y}.{z}.{c}'
                pf_file.write(f'''  <link id="{name}" bandwidth="{link_bw}" latency="{link_lat}"/>\n''')

pf_file.write('''<!-- Route spec -->\n''')

pf_file.write('''<!-- Intra node inter core -->\n''')
for x in range(xdim):
    for y in range(ydim):
        for z in range(zdim):
            for c in range(1,num_cpus):
                name = f'link-{x}.{y}.{z}.{0}-{x}.{y}.{z}.{c}'
                pf_file.write(f'''  <route src="node-{x}.{y}.{z}.c{0}" dst="node-{x}.{y}.{z}.c{c}">
    <link_ctn id="{name}"/>
  </route>
''')

pf_file.write('<!-- Inter node connections -->\n')
for (k,v) in routes.items():
    if (k[0] == k[1]):
        # loopback route
        pass
    else:
        pf_file.write(f'''  <route src="{k[0]}.c0" dst="{k[1]}.c0">\n''')
        for i in range(len(v)-1):
            if ((v[i], v[i+1]) in link_names):
                pf_file.write(f'''    <link_ctn id="{link_names[(v[i], v[i+1])]}"/>\n''')
            else:
                pf_file.write(f'''    <link_ctn id="{link_names[(v[i+1], v[i])]}"/>\n''')
        pf_file.write(f'''  </route>\n''')

pf_file.write('''  </zone>
</platform>
''')

pf_file.close()

####################### CREATE DEPLOYMENT FILE ##########################

dep_file = open(f'deployment-{xdim}.{ydim}.{zdim}.{num_cpus}-torus.xml','w+')

# MAKE THE NODE AT THE CENTER AS SLURMCTLD AND OTHER NODES AS SLURMD

dep_file.write('''<?xml version='1.0'?>
<!DOCTYPE platform SYSTEM "https://simgrid.org/simgrid.dtd">
<platform version="4.1">
''')
dep_file.write('  <!-- SlurmCtlD -->\n')
dep_file.write(f'  <actor host="node-{xdim//2}.{ydim//2}.{zdim//2}.c0" function="slurmctld">\n')
dep_file.write(f'    <argument value="jobs.csv"/>\n')
dep_file.write(f'    <argument value="fcfs_backfill"/>\n')
for x in range(xdim):
    for y in range(ydim):
        for z in range(zdim):
            dep_file.write(f'    <argument value="node-{x}.{y}.{z}.c0"/>\n')
dep_file.write(f'  </actor>\n')

dep_file.write('  <!-- SlurmDs -->\n')
for x in range(xdim):
    for y in range(ydim):
        for z in range(zdim):
            dep_file.write(f'  <actor host="node-{x}.{y}.{z}.c0" function="slurmd">\n')
            dep_file.write(f'    <argument value="node-{xdim//2}.{ydim//2}.{zdim//2}.c0"/>\n')
            for c in range(num_cpus):
                dep_file.write(f'    <argument value="node-{x}.{y}.{z}.c{c}"/>\n')
            dep_file.write('  </actor>\n')
dep_file.write('</platform>')