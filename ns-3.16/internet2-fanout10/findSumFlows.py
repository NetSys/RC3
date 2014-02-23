import sys

f = open(sys.argv[1])

lines = f.readlines()

flowsize = 0
count = 0
for line in lines[1:]:
  words = line.split()
  flowsize = flowsize + float(words[1])

print flowsize
print count
