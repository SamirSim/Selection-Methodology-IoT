import sys
import ast
import csv

trace = sys.argv[1]
output = sys.argv[2]

with open(trace) as file: #First loop for filling the dict elements (id, mac, ip...)
	line = True
	while line:
		line = file.readline()
		if line:
			dict = line.split(' ', 1)[1]
			print(dict)
			elem = ast.literal_eval(dict)
			data_rate = line.split()[0]
			total_power = elem["totalPower"]
			idle = elem["idle"]
			transmission = elem["transmission"]
			reception = elem["reception"]
			cross_factor_tx = elem["cross-factor-tx"]
			cross_factor_rx = elem["cross-factor-rx"]

			with open(output, 'a', newline='') as file2:
				write = csv.writer(file2)
				write.writerow([data_rate, total_power, idle, transmission, reception, cross_factor_tx, cross_factor_rx])