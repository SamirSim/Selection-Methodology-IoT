import csv
from operator import truediv
import sys

filepath = sys.argv[1]

dict = []
cpt = 0.0
time = 0.0

with open(filepath) as fp:
   line = fp.readline()
   while line:
       words = line.split()
       time += float(words[len(words)-1])
       cpt = cpt + 1
       line = fp.readline()

print(time/cpt)

"""
with open(filepath) as fp:
   line = fp.readline()
   while line:
       words = line.split()
       id = words[len(words)-1].replace(')', '')
       if "EndDeviceLorawanMac:Send(" in line:
           start = float(words[0].replace('s', '').replace('+', ''))
           element = {"_id": id, "start": start, "end": ""}
           dict.append(element)
           line = fp.readline()
       else:
           id = words[len(words)-1].replace(')', '')
           end = float(words[0].replace('s', '').replace('+', ''))
           line = fp.readline()
           for element in dict:
               if element["_id"] == id:
                   start = element["start"]
                   stop = 1
                   break
       if stop:
            break
print ("{:.4f}".format(end-start))
"""

