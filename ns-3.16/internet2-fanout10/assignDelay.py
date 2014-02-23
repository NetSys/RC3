import sys

topo = open(sys.argv[1])
endhost = open(sys.argv[2])
out = open(sys.argv[3], 'w')

lines = endhost.readlines()

endhosts = list()
for line in lines:
  endhosts.append(int(line.split()[0]))

lines = topo.readlines()

out.write(lines[0])
out.write(lines[1])
for line in lines[2:]:
  words = line.split()
  if((int(words[0]) in endhosts) or (int(words[1]) in endhosts)):
    delay = 1
  else:
    delay = 10
  out.write(words[0] + "\t" + words[1] + "\t" + words[2] + "\t" + str(delay) + "\n")


