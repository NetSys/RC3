import sys
import random

inpfile = open(sys.argv[1])
endtime = float(sys.argv[2])

lines = inpfile.readlines()





totalBytes = dict()

for line in lines[1:]:
  words = line.split()
  size = int(words[1])
  node1 = int(words[2])
  if(node1 not in totalBytes.keys()):
    totalBytes[node1] = 0
  totalBytes[node1] += size

linkutil = dict()
for key in totalBytes.keys():
  linkutil[key] = float(totalBytes[key]*8*100)/float(1000000000*endtime)
  print str(key) + "\t" + str(linkutil[key])



