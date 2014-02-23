import sys

inpfile = open(sys.argv[1])
fanout = int(sys.argv[2])
outfile = open(sys.argv[3],'w')

lines = inpfile.readlines()




num_nodes = int(lines[0].split()[0])
num_corenodes = num_nodes/2
num_edgenodes = num_corenodes*fanout
num_nodes = num_corenodes + num_edgenodes

num_links_org = int(lines[1].split()[0])
num_core_links = num_links_org - num_corenodes
num_links = num_core_links + num_edgenodes

outfile.write(str(num_nodes) + "\n")
outfile.write(str(num_links) + "\n")
for line in lines[2:]:
  words = line.split()
  node1 = int(words[0])
  node2 = int(words[1])
  bw = int(words[2])
  delay = int(words[3])
  if(node2 >= num_corenodes):
    for i in xrange (0, fanout):
      outfile.write(str(node1) + "\t" + str(node2+(i*num_corenodes)) + "\t" + str(bw/fanout) + "\t" + str(delay) + "\n")
  else:
    outfile.write(line)
outfile.close()



