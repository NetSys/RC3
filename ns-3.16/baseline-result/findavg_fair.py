import os
import sys
import numpy


regular = dict()
rc3 = dict()


def findavgregular(input_file):
  lines = input_file.readlines()
  for line in lines:
      words  = line.split()
      if((float(words[1]) + float(words[2]) <= 10) and (float(words[2]) >= 2)):
            if(int(words[0]) not in regular.keys()):
              regular[int(words[0])] = dict()
            regular[int(words[0])][int(words[3])] = float(words[1])


def findavgrc3(input_file):
  lines = input_file.readlines()
  for line in lines:
      words  = line.split()
      if((float(words[1]) + float(words[2]) <= 5) and (float(words[2]) >= 2)):
            if(int(words[0]) not in rc3.keys()):
              rc3[int(words[0])] = dict()
            rc3[int(words[0])][int(words[3])] = float(words[1])

if __name__ == "__main__":
  if len(sys.argv) < 5:
    print "Usage:", sys.argv[0], "<input_file1> <input_file2> <outputfile1 format> <outputfile2 format>" 
    sys.exit(1)

  try:
    input_file1 = open(sys.argv[1])
    input_file2 = open(sys.argv[2])
  except:
    print "Error opening", sys.argv[1]
    sys.exit(1)

  findavgregular(input_file1)
  findavgrc3(input_file2)

  regularlatencies = dict()
  rc3latencies = dict()

  regularbulk = list()
  rc3bulk = list()
  
  
  regularweighted = list()
  rc3weighted = list()

  sizes = list()

  count  = 0
  for size in rc3.keys():
    
    regularlatencies[size] = list()
    rc3latencies[size] = list()
    
    for key in rc3[size].keys():
        if(key in regular[size].keys()):

            #count = count + 1
            #print "found common" + str(count)

            regularlatencies[size].append(regular[size][key])
            rc3latencies[size].append(rc3[size][key])
            
            regularbulk.append(regular[size][key])
            rc3bulk.append(rc3[size][key])

            regularweighted.append(regular[size][key]*float(size))
            rc3weighted.append(rc3[size][key]*float(size))
            sizes.append(float(size))

  f1 = open("improvements/regular-"+sys.argv[3], 'w')
  f2 = open("improvements/regular-bulk-"+sys.argv[3], 'w')
  f3 = open("improvements/regular-weighted-"+sys.argv[3], 'w')
  
  for key in sorted(regularlatencies.keys()):
      avg = sum(regularlatencies[key])/len(regularlatencies[key])
      med = numpy.percentile(regularlatencies[key], int(50))
      perc99 = numpy.percentile(regularlatencies[key], int(99))
      perc1 = numpy.percentile(regularlatencies[key], int(1))
      perc10 = numpy.percentile(regularlatencies[key], int(10))
      length = len(regularlatencies[key])
      f1.write(str(key) + "\t" + str(avg) + "\t" + str(med) + "\t" + str(perc99) + "\t" + str(perc1) + "\t" + str(perc10) + "\t" + str(length) + "\n")
   
  avg = sum(regularbulk)/len(regularbulk)
  med = numpy.percentile(regularbulk, int(50))
  perc99 = numpy.percentile(regularbulk, int(99))
  perc1 = numpy.percentile(regularbulk, int(1))
  perc10 = numpy.percentile(regularbulk, int(10))
  length = len(regularbulk)
  f2.write("avg_over_flows" + "\t" + str(avg) + "\t" + str(med) + "\t" + str(perc99) + "\t" + str(perc1) + "\t" + str(perc10) + "\t" + str(length) + "\n")
  
  avg = sum(regularweighted)/sum(sizes)
  med = numpy.percentile(regularweighted, int(50))
  perc99 = numpy.percentile(regularweighted, int(99))
  perc1 = numpy.percentile(regularweighted, int(1))
  perc10 = numpy.percentile(regularweighted, int(10))
  length = len(regularweighted)
  f3.write("avg_over_bytes" + "\t" + str(avg) + "\t" + str(med) + "\t" + str(perc99) + "\t" + str(perc1) + "\t" + str(perc10) + "\t" + str(length) + "\n")
  
  f1.close()
  f2.close()
  f3.close()

  f1 = open("improvements/rc3-"+sys.argv[4], 'w')
  f2 = open("improvements/rc3-bulk-"+sys.argv[4], 'w')
  f3 = open("improvements/rc3-weighted-"+sys.argv[4], 'w')

  for key in sorted(rc3latencies.keys()):
      avg = sum(rc3latencies[key])/len(rc3latencies[key])
      med = numpy.percentile(rc3latencies[key], int(50))
      perc99 = numpy.percentile(rc3latencies[key], int(99))
      perc1 = numpy.percentile(rc3latencies[key], int(1))
      perc10 = numpy.percentile(rc3latencies[key], int(10))
      length = len(rc3latencies[key])
      f1.write(str(key) + "\t" + str(avg) + "\t" + str(med) + "\t" + str(perc99) + "\t" + str(perc1) + "\t" + str(perc10) + "\t" + str(length) + "\n")

  avg = sum(rc3bulk)/len(rc3bulk)
  med = numpy.percentile(rc3bulk, int(50))
  perc99 = numpy.percentile(rc3bulk, int(99))
  perc1 = numpy.percentile(rc3bulk, int(1))
  perc10 = numpy.percentile(rc3bulk, int(10))
  length = len(rc3bulk)
  f2.write("avg_over_flows" + "\t" + str(avg) + "\t" + str(med) + "\t" + str(perc99) + "\t" + str(perc1) + "\t" + str(perc10) + "\t" + str(length) + "\n")
  
  avg = sum(rc3weighted)/sum(sizes)
  med = numpy.percentile(rc3weighted, int(50))
  perc99 = numpy.percentile(rc3weighted, int(99))
  perc1 = numpy.percentile(rc3weighted, int(1))
  perc10 = numpy.percentile(rc3weighted, int(10))
  length = len(regularweighted)
  f3.write("avg_over_bytes" + "\t" + str(avg) + "\t" + str(med) + "\t" + str(perc99) + "\t" + str(perc1) + "\t" + str(perc10) + "\t" + str(length) + "\n")

  f1.close()
  f2.close()
  f3.close()

