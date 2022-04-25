import csv
import sys

#filepath = 'parsedFile.txt'
filepath = sys.argv[1]
output = sys.argv[2]

with open(filepath) as fp:
	line = fp.readline() #Remove the four first lines (build lines)
	
	#while (line[0].isnumeric() == False):
		#print(line[0])
	#	line = fp.readline()

	while line:
		words = line.split()
		nWifi = float(words[0])
		throughput = float(words[1])
		with open(output, 'a', newline='') as file:
			writer = csv.writer(file)
			writer.writerow([nWifi, throughput])
		line = fp.readline()