#!/usr/bin/env python

import sys
import os
import matplotlib
matplotlib.use('MacOSX')
import matplotlib.font_manager as fm
import matplotlib.pyplot as plt
from matplotlib.ticker import FormatStrFormatter

import numpy as np
import os
import sys


params = {'figure.figsize'  : [8.5, 3], 
          'font.family': 'MankSans-Medium',
          'grid.color': '#aaaaaa',
          #'grid.linestyle': '--',
          #'grid.linewidth': 1.5
          }     
          
def main():
  if len(sys.argv) < 3:
    print "Usage:", sys.argv[0], "<input_file1> <input_file2> <output_file>"
    sys.exit(1)

  try:
    input_file1 = open(sys.argv[1])
    input_file2 = open(sys.argv[2])
    output_file = sys.argv[3]
  except:
    print "Error opening", sys.argv[1]
    sys.exit(1)

  lines1 = input_file1.readlines()
  lines2 = input_file2.readlines()
  
  X = []
  Y1 = []
  Y2 = []

  for i in xrange(0, len(lines1)):
    splits = lines1[i].split()
    X.append(lines1[i].split()[0])
    Y1.append(float(lines1[i].split()[1]))
    Y2.append(float(lines2[i].split()[1]))


  N = len(X)

  ind = np.arange(N)
  width = 0.25


  plt.rcParams.update(params)
  fig = plt.figure()  
  ax1 = fig.add_subplot(111)

  ax1.set_ylabel('% Reduction in avg FCT', fontsize=20)
  rects1 = ax1.bar(ind, Y1,  width, color = '#990000', edgecolor='white', label="Theoretical Result")
  rects2 = ax1.bar(ind+width, Y2, width, color = '#999999', edgecolor='white', label="Experimental Result")
  handles, labels = ax1.get_legend_handles_labels()
  
  ax1.set_xticks(ind+width)
  ax1.set_xticklabels(X, rotation=25)
  ax1.tick_params(axis='x', which='major', labelsize=14)
  ax1.set_xlabel('Flow Size (bytes)', fontsize=20)
  ax1.tick_params(axis='y', which='major', labelsize=14)
  handles, labels = ax1.get_legend_handles_labels()
  ax1.legend(handles, labels, loc=2, fontsize=16)
  ax1.grid(True) 
  
  plt.savefig(output_file + ".pdf", format="pdf", bbox_inches='tight')
  

if __name__ == "__main__":
  main()
