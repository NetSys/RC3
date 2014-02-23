import sys

linkFile = open(sys.argv[1])
endHostFile = open(sys.argv[2])

lines = linkFile.readlines()

adjList = dict()

for line in lines[2:]:
  words = line.split()
  node1 = int(words[0])
  node2 = int(words[1])
  delay = int(words[3])
  if node1 not in adjList.keys():
    adjList[node1] = list()
  adjList[node1].append((node2, delay))
  if node2 not in adjList.keys():
    adjList[node2] = list()
  adjList[node2].append((node1, delay))


print len(adjList)
num_nodes = len(adjList)
distList = dict()
hopList = dict()

#initializing distance and hops list
for node in sorted(adjList.keys()):
  distList[node] = dict()
  hopList[node] = dict()
  for i in xrange(0, num_nodes):
    if(i==node):
      distList[node][i] = 0
      hopList[node][i] = 0
    else:
      distList[node][i] = 100000
      hopList[node][i] = 100000
  for x in adjList[node]:
    distList[node][x[0]] = x[1]
    hopList[node][x[0]] = 1


#Floyd Warshall
for k in xrange(0, num_nodes):
  for i in xrange(0, num_nodes):
    for j in xrange(0, num_nodes):
      if(hopList[i][j] > hopList[i][k] + hopList[k][j]):
        hopList[i][j] = hopList[i][k] + hopList[k][j]
        distList[i][j] = distList[i][k] + distList[k][j]



#for node in hopList.keys():
#  for x in hopList[node].keys():
#    print str(node) + "->" + str(x) + ":\t" + str(hopList[node][x])

lines = endHostFile.readlines()
endhosts = list()
for line in lines[1:]:
  endhosts.append(int(line.split()[0]))

delays = list()
for node in distList.keys():
  for x in distList[node].keys():
    if((node in endhosts) and (x in endhosts)):
        delays.append(distList[node][x])
        print str(node) + "->" + str(x) + ":\t" + str(distList[node][x])

print max(delays)
print min(delays)
print sum(delays)/len(delays)
