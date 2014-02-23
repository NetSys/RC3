import sys

if(len(sys.argv) < 2):
  print "<input> <output>"

f = open(sys.argv[1])

lines = f.readlines()

nodes = dict()
count  = 0
for line in lines[2:]:
  words = line.split()
  for i in xrange(0,2):
    if(int(words[i]) not in nodes.keys()):
      nodes[int(words[i])] = list()
      nodes[int(words[i])].append((count,i))
    else:
      nodes[int(words[i])].append((count,i))
  count = count + 1

endhosts = list()
for key in nodes.keys():
  if(len(nodes[key]) == 1):
    print str(key) + "\t" + str(nodes[key][0])
    endhosts.append((key,nodes[key][0]))

f2 = open(sys.argv[2], 'w')
f2.write(str(len(endhosts))+"\n")
for x in endhosts:
  f2.write(str(x[0]) + "\t" + str(x[1][0]) + "\t" + str(x[1][1]) + "\n")
