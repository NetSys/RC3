import sys

def findavgfct(inpfile):
  fct = dict()
  lines = inpfile.readlines()
  for line in lines:
    words = line.split()
    key = words[0]
    fct[key] = (words[0], float(words[1]))
  return fct


if(len(sys.argv) < 3):
  print "<input1> <input2> <output-pattern>"

inpfile = open(sys.argv[1])
avgregular = findavgfct(inpfile)
inpfile = open(sys.argv[2])
avgrc3 = findavgfct(inpfile)

f = open("avgreduction-" + sys.argv[3] + ".txt", 'w')
for key in avgregular.keys():
    imp = (avgregular[key][1] - avgrc3[key][1])*100 / avgregular[key][1]
    f.write(str(avgregular[key][0]) + "\t" + str(imp) + "\n")
    print "Regular TCP: " + str(avgregular[key][1]) + "; RC3: " + str(avgrc3[key][1]) + "; Reduction: " + str(imp)



    
  
