import re
import matplotlib.pyplot as plt
import numpy as np

filename = "output_sdram.txt"
plotdata = open(filename)

plotdata.readline()
plotdata.readline()
plotdata.readline()
plotdata.readline()

linenum = 0
time = []
for line in plotdata:
    if linenum == 511:
        break
    data = re.split(r'\t',plotdata.readline())
    tmp = (re.split('\s',data[3])[2])
    if tmp != "":
        time.append(float(tmp))
    linenum += 1

#print(time)
size = np.arange(1,linenum-1)
plotdata.close()


#fig, ax = plt.subplot(1)
#ax.xaxis.set_major_locator(plt.MaxNLocator(3))
plt.plot(size,time,'ro')
plt.show()
