#!/usr/bin/env python
import commands
import sys
import os

threads=[1,2,3,4,5,6,7,8,9,10]
chunk_size=[512,1*1024,4*1024,8*1024,16*1024,32*1024,64*1024,128*1024,256*1024,512*1024,1*1024*1024,2*1024*1024]
types={
        1 : "fast_pipe",
        2 : "fast_event",
        3 : "fast_spin",
        4 : "raw_pipe"
}

status, output = commands.getstatusoutput("id -u")
if int(output) != 0:
    print "Please run in sudo "
    sys.exit(0)

def run_cmd(t, s, n, cmd):
        f = open("result.txt", "a+")
        status, output = commands.getstatusoutput(cmd)
        lines = output.split()
        median = []
        for line in lines:
                vals = line.split(",")
                median.append(int(vals[3].strip()))
        f.write("{0},{1},{2},{3}\n".format(t, s, n, median[len(median)/2]))
        f.close()
        sys.stdout.write("{0}\n".format(median[len(median)/2]))

loops = 5

total = len(threads) * len(chunk_size) * len(types)
total_time = total * loops
for thread in threads:
        for size in chunk_size:
                for ipc in types.keys():
                        cmd = "LD_LIBRARY_PATH=\".\" ./perf.test threads={0} size={1} type={2} loops={3} nohead=1".format(thread, size, ipc, loops)
                        total_time = total_time - loops
                        sys.stdout.write(cmd + " (left {0} seconds) => ".format(total_time))
                        run_cmd(thread, size, types[ipc], cmd)
