###Reproducing RC3 Baseline Results

Here are the instructions for reproducing the baseline RC3 result (on simplified Internet-2 topology, with bottleneck bandwidth of 1Gbps, average RTT of 40ms at 30% average link utilization)

1. Clone the rc3 repository
2. cd ns-3.16
3. ./waf configure -d release  
4. ./waf
5. ./run-baseline #runs the simulation with regular TCP, followed by RC3. It takes around 2 hours for each simulation to complete
6. cd baseline-result
7. ./getres-fair  #computes the average FCTs and stores them in improvements/ 
8. cd improvements
9. ./getimp       #computes the % reduction and prints the results
10. ./getplot-report #plots the baseline graph

Please contact us at (radhika @ eecs dot berkeley dot edu) or (justine @ eecs dot berkeley dot edu)
- if you would like to test RC3 under more scenarios as done in the paper. We can send you the workloads and topologies we used, along with further instructions.
- if you happen to find a bug! We have extensively tested RC3 and hope it is bug-free. But if there is a corner-case that we failed to take into account, please bring it to our notice.
- if you have any other questions
