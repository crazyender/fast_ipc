#!/usr/bin/env python
import commands
import sys
import os

threads=[1,2,4,6,8]
chunk_size=[512,4*1024,16*1024,64*1024,256*1024,1*1024*1024]

types={
        1 : "fast_pipe",
        2 : "fast_event",
        3 : "fast_spin",
        4 : "raw_pipe"
}

if os.path.exists("result.txt"):
        os.remove("result.txt")

status, output = commands.getstatusoutput("id -u")
if int(output) != 0:
    print "Please run in sudo "
    sys.exit(0)

def run_cmd(t, s, n, cmd):
        status, output = commands.getstatusoutput(cmd)
        lines = output.split()
        median = []
        for line in lines:
                vals = line.split(",")
                median.append(int(vals[3].strip()))
        
        return median[len(median)/2]

loops = 5

total = len(threads) * len(chunk_size) * len(types)
total_time = total * loops
f = open("result.txt", "a+")
for thread in threads:
        for ipc in types.keys():
                max_val = 0
                max_in_size = 0
                for size in chunk_size:
                        sys.stdout.write("\rrun at threads {0} type {1} (left {2} seconds) => ".format(thread, types[ipc], total_time))
                        sys.stdout.flush()
                        cmd = "LD_LIBRARY_PATH=\".\" ./perf.test threads={0} size={1} type={2} loops={3} nohead=1".format(thread, size, ipc, loops)
                        total_time = total_time - loops
                        val = run_cmd(thread, size, types[ipc], cmd)
                        if val > max_val:
                                max_val = val
                                max_in_size = size
                
                f.write("{0},{1},{2},{3}\n".format(thread, max_in_size, types[ipc], max_val))
                sys.stdout.write("type:{0} at {1} value {2}\n".format(types[ipc], max_in_size, max_val))
                f.flush()
f.close()
