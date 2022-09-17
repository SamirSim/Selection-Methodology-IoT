import csv
import sys

filepath = sys.argv[1]
output = sys.argv[2]
number = sys.argv[3]

with open(filepath, "r") as file:
    first_line = file.readline()
    for last_line in file:
        pass

words = last_line.split()
sent = float(words[0])
received = float(words[1])

ratio = (received/sent*100)
with open(output, 'a', newline='') as file:
    writer = csv.writer(file)
    writer.writerow([number, ratio])