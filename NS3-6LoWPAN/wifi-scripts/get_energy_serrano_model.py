import pprint
import sys

trace = sys.argv[1]
simulation_time = float(sys.argv[2])
packet_size = str(int(sys.argv[3])+28) #Header Size = 28
device_model = sys.argv[4]
mcs = int(sys.argv[5])
tx_power = int(sys.argv[6])
data_rate = sys.argv[7]

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
							if dict['time_frame']==0 or dict['time_ack']==0:
								begin = float(words[1])
							dict['n_g'] = dict['n_g'] + 1 
							if packet_size in words:
								dict['n_at'] = dict['n_at'] + 1 
							if '(CTL_ACK' in words:
								dict['n_rx'] = dict['n_rx'] + 1 
				else:
					id = words[2].split('/')[2]
					for dict in dicts:
						if dict['id']==id:
							if packet_size in words:
								#dict['n_at'] = dict['n_at'] + 1 
								if dict['time_frame']==0:
									time_frame = float(words[1]) - begin
							if '(CTL_ACK' in words:
								if dict['time_ack']==0:
									time_ack = float(words[1]) - begin
								mac = words[words.index('ns3::WifiMacTrailer')-1].replace('R','').replace('A','').replace(')','').replace('=','')
								if dict['mac'] == mac: 
									dict['n_tx'] = dict['n_tx'] + 1 

if device_model == 'Soekris': #We consider this device model only, and also that the all the devices are of the same model
	rho_id = 3.56
	gamma_xg = 0.93
	gamma_xr = 0.93
	if mcs == 1:
		rho_rx = 0.27
		if tx_power == 6:
			rho_tx = 0.33
		elif tx_power == 9:
			rho_tx = 0.57
		elif tx_power == 12:
			rho_tx = 0.7
		elif tx_power == 15:
			rho_tx = 0.86
	elif mcs == 0:
		rho_rx = 0.16
		if tx_power == 6:
			rho_tx = 0.30
		elif tx_power == 9:
			rho_tx = 0.59
		elif tx_power == 12:
			rho_tx = 0.73
		elif tx_power == 15:
			rho_tx = 0.89
	elif mcs == 2:
		rho_rx = 0.6
		if tx_power == 6:
			rho_tx = 0.36
		elif tx_power == 9:
			rho_tx = 0.88
		elif tx_power == 12:
			rho_tx = 1.02
		elif tx_power == 15:
			rho_tx = 1.17
	elif mcs == 3:
		rho_rx = 1.14
		if tx_power == 6:
			rho_tx = 1.2
		elif tx_power == 9:
			rho_tx = 1.24
		elif tx_power == 12:
			rho_tx = 1.37
		elif tx_power == 15:
			rho_tx = 1.58

totalPower = {'totalPower':0, 'idle':0, 'transmission':0, 'reception':0, 'cross-factor-tx':0, 'cross-factor-rx':0}

for dict in dicts:
	dict['time_frame'] = time_frame
	dict['time_ack'] = time_ack

	dict['lambda_g'] = dict['n_g'] / simulation_time
	dict['lambda_r'] = dict['n_rx'] / simulation_time

	dict['tau_tx'] = (dict['n_at'] * dict['time_frame'] + dict['n_rx'] * dict['time_ack']) / simulation_time
	dict['tau_rx'] = (dict['n_rx'] * dict['time_frame'] + dict['n_tx'] * dict['time_ack']) / simulation_time

	dict['power'] = rho_id + rho_tx * dict['tau_tx'] + rho_rx * dict['tau_rx'] + gamma_xg * dict['lambda_g'] * 0.001 + gamma_xr * dict['lambda_r'] * 0.001
	
	totalPower['totalPower'] = dict['power']
	totalPower['idle'] = rho_id
	totalPower['transmission'] = rho_tx * dict['tau_tx']
	totalPower['reception'] = rho_rx * dict['tau_rx']
	#totalPower['cross-factor-tx'] = gamma_xg * dict['lambda_g'] * 0.001
	#totalPower['cross-factor-rx'] = gamma_xr * dict['lambda_r'] * 0.001

#print(dicts, totalPower)
print(data_rate, totalPower)
