import pprint
import sys
import csv

trace = sys.argv[1]
buffer = sys.argv[2]
simulation_time = float(sys.argv[3])
packet_size = str(int(sys.argv[4])+28) #Header Size = 28
device_model = sys.argv[5]
energyRatio = sys.argv[6]
nbRxPackets = float(sys.argv[7])
nbTxPackets = float(sys.argv[8])
output = sys.argv[9]

dicts = []
ids = [] 
 
with open(trace) as file: #First loop for filling the dict elements (id, mac, ip...)
	line = True
	while line:
		line = file.readline()
		if line:
			words = line.split()
			time = words[1]
			if (words[0] == 't') or (words[0] == 'r'):
				if words[0] == 't':
					id = words[2].split('/')[2]
					if id not in ids:
						ids.append(id)
						dict = {'id': id, 'mac':'', 'ip':'', 'n_g': 0, 'n_at': 0, 'n_rx':0, 'n_tx': 0, 'time_frame': 0, 'time_ack': 0, 'lambda_g': 0, 'lambda_r':0, 'tau_rx': 0, 'tau_tx': 0, 'power':0}
						dicts.append(dict)
					else:
						for dict in dicts:
							if dict['id']==id:
								if packet_size in words:
									if dict['ip'] == '':
										dict['ip'] = words[words.index('length:')+2]
								if 'mac:' in words and 'length' in words:
									if dict['mac'] == '':
										dict['mac'] = words[words.index('mac:')+1][6:]


with open(trace) as file: #Second loop for updating the values (n_g, n_at...)
	line = True
	while line:
		line = file.readline()
		if line:
			words = line.split()
			time = words[1]
			if (words[0] == 't') or (words[0] == 'r'):
				if words[0] == 't':
					id = words[2][10]
					for dict in dicts:
						if dict['id']==id:
							dict['n_g'] = dict['n_g'] + 1 
							if '(CTL_ACK' in words:
								dict['n_rx'] = dict['n_rx'] + 1 

if device_model == 'Soekris': #We consider this device model only, and also that the all the devices are of the same model
	gamma_xg = 0.93
	gamma_xr = 0.93

totalPower = {'totalPower':0, 'idle':0, 'transmission':0, 'reception':0, 'cross-factor-tx':0, 'cross-factor-rx':0}

#print(dicts)

for dict in dicts:
	if dict['id'] == '0':
		if energyRatio=="1":
			dict['lambda_g'] = nbTxPackets # Number of generated packets by the station (ED), in Joules
			dict['lambda_r'] = dict['n_rx'] # Number of received packets (or frames) by the station (ED), in Joules

			totalPower['cross-factor-tx'] = gamma_xg * dict['lambda_g'] * 0.001 / nbRxPackets # Divided by the number of received bytes in AP if upstream, ED if downstream, Joules per Byte
			totalPower['cross-factor-rx'] = gamma_xr * dict['lambda_r'] * 0.001 / nbRxPackets # Divided by the number of received bytes in AP if upstream, ED if downstream, Joules per Byte
		else:
			dict['lambda_g'] = nbTxPackets / simulation_time #in Watts
			dict['lambda_r'] = dict['n_rx'] / simulation_time #in Watts

			totalPower['cross-factor-tx'] = gamma_xg * dict['lambda_g'] * 0.001 # Energy in Watts
			totalPower['cross-factor-rx'] = gamma_xr * dict['lambda_r'] * 0.001  # Energy in Watts

		
f = open(buffer, "r")
buffer = (f.read().replace('\n', ''))

with open(output, 'a', newline='') as file2:
	write = csv.writer(file2)
	print(buffer, totalPower['cross-factor-tx'], totalPower['cross-factor-rx'])
	write.writerow([buffer, totalPower['cross-factor-tx'], totalPower['cross-factor-rx']])