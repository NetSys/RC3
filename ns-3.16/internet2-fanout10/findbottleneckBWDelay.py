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
  bw = int(words[2])
  if node1 not in adjList.keys():
    adjList[node1] = list()
  adjList[node1].append((node2, delay, bw))
  if node2 not in adjList.keys():
    adjList[node2] = list()
  adjList[node2].append((node1, delay, bw))


print len(adjList)
num_nodes = len(adjList)
distList = dict()
hopList = dict()
bwList = dict()

#initializing distance and hops list
for node in sorted(adjList.keys()):
  distList[node] = dict()
  hopList[node] = dict()
  bwList[node] = dict()
  for i in xrange(0, num_nodes):
    if(i==node):
      distList[node][i] = 0
      hopList[node][i] = 0
      bwList[node][i] = 1000000000
    else:
      distList[node][i] = 100000
      hopList[node][i] = 100000
      bwList[node][i] = 1000000000
  for x in adjList[node]:
    distList[node][x[0]] = x[1]
    hopList[node][x[0]] = 1
    bwList[node][x[0]] = x[2]



#Floyd Warshall
for k in xrange(0, num_nodes):
  for i in xrange(0, num_nodes):
    for j in xrange(0, num_nodes):
      if(hopList[i][j] > hopList[i][k] + hopList[k][j]):
        hopList[i][j] = hopList[i][k] + hopList[k][j]
        distList[i][j] = distList[i][k] + distList[k][j]
        if(bwList[i][k] < bwList[i][j]):
          bwList[i][j] = bwList[i][k]
        if(bwList[k][j] < bwList[i][j]):
          bwList[i][j] = bwList[k][j]




#for node in hopList.keys():
#  for x in hopList[node].keys():
#    print str(node) + "->" + str(x) + ":\t" + str(hopList[node][x])

lines = endHostFile.readlines()
endhosts = list()
for line in lines[1:]:
  endhosts.append(int(line.split()[0]))

delays = list()
bottleneckBWs = dict()
for node in distList.keys():
  for x in distList[node].keys():
    if((node in endhosts) and (x in endhosts)):
      if((node%10) != (x%10)):
        delays.append(distList[node][x])
        if(bwList[node][x] < 1000000000):
          if(bwList[node][x] not in bottleneckBWs.keys()):
            bottleneckBWs[bwList[node][x]] = 1
          else:
            bottleneckBWs[bwList[node][x]] += 1

        print str(node) + "->" + str(x) + ":\t" + str(distList[node][x]) + "\t" + str(bwList[node][x])

print max(delays)
print min(delays)
print sum(delays)/len(delays)

print bottleneckBWs
