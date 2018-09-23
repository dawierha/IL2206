import re
import matplotlib.pyplot as plt
import numpy as np

filenames = ["output_sdram.txt", "output_onchip_memory.txt","output_sram.txt"]

for filename in filenames:

    plotdata = open(filename)

    plotdata.readline()
    plotdata.readline()
    plotdata.readline()
    plotdata.readline()

    linenum = 0
    time = []
    size = []

    dotindex = 0
    dots = ['r.', 'b.','g.','k.','c.','m.']

    plt.figure()
    for line in plotdata:
        if linenum == 511:
            linenum = 0
            plt.plot(size,time,dots[dotindex])
            time = []
            size = []
            dotindex += 1
        data = re.split(r'\t',line)
        tmp = (re.split('\s',data[3])[2])
        tmp2 = (re.split('\s',data[0])[1])
        if tmp != "":
            time.append(int(tmp))
            size.append(int(tmp2))
        linenum += 1

    plotdata.close()

plt.show()
