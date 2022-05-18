import csv
import sys

filepath = sys.argv[1]

total = 0
cpt = 0

with open(filepath) as fp:
	line1 = fp.readline()
	line2 = fp.readline()
	while line2:
		if "UdpEchoServerApplication" in line2 and "UdpEchoClientApplication" in line1:
			words1 = line1.split()
			words2 = line2.split()
			start = float(words1[0].replace('s', '').replace('+', ''))  #Time in logs is in Micro Seconds
			end = float(words2[0].replace('s', '').replace('+', '')) 
			cpt = cpt + 1
			total = total + (end-start)
			line1 = fp.readline()
			line2 = fp.readline()
		else:
			line1 = line2
			line2 = fp.readline()

#print(total/cpt)
print ("{:.5f}".format(total/cpt))