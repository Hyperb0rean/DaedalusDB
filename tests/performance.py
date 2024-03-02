import matplotlib.pyplot as plt 
import csv 
import sys

  
x = [] 
y = [] 
  
with open(sys.argv[1] +'.csv','r') as csvfile: 
    lines = csv.reader(csvfile, delimiter=',') 
    for row in lines: 
        x.append(int(row[0])) 
        y.append(int(row[1])) 
  
plt.plot(x, y, color = 'g', linestyle = 'None', 
         marker = '.') 
  
if sys.argv[1] == 'Compression':
    plt.ylabel('Filesize (bytes)') 
else:
    plt.ylabel('Time (μs)') 

plt.xlabel('Elements') 
plt.title(sys.argv[1], fontsize = 20) 
plt.grid() 
plt.legend() 
plt.show() 