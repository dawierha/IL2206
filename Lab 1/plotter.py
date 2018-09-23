import re
import matplotlib.pyplot as plt

filename = "output_sdram.txt"
plotdata = open(filename)

for line in plotdata:
    data = re.split(r'\t',plotdata.readline())
    plt.plot(re.split('\s+',data(3))(1))


plt.show()
