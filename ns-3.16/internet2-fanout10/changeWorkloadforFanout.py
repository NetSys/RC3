import sys
import random

inpfile = open(sys.argv[1])
fanout = int(sys.argv[2])
num_corenodes = int(sys.argv[3])
outfile = open(sys.argv[4],'w')

lines = inpfile.readlines()





outfile.write(str(len(lines))+"\n")

for line in lines[0:]:
  words = line.split()
  starttime = words[0]
  size = words[1]
  node1 = int(words[2])
  node2 = int(words[3])
  r = random.randint(0, fanout-1)
  node1 = node1 + (r*num_corenodes)
  r = random.randint(0, fanout-1)
  node2 = node2 + (r*num_corenodes)
  outfile.write(starttime + "\t" + size + "\t" + str(node1) + "\t" + str(node2) + "\n")
outfile.close()



