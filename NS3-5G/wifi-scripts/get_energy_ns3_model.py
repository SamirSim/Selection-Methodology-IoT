import pprint
import sys

trace = sys.argv[1]
simulation_time = float(sys.argv[2])
nWifi = sys.argv[3]
energyRatio = sys.argv[4]
nbPackets = float(sys.argv[5])

dicts = []
ids = [] 
energy = 0
 
with open(trace) as file: #First loop for filling the dict elements (id, mac, ip...)
	line = file.readline()
	words = line.split()
	while ("Tx" not in words):
		line = file.readline()
		words = line.split()

	tx_energy = float(words[-1])

	line = file.readline()
	words = line.split()
	rx_energy = float(words[-1])

	line = file.readline()
	words = line.split()
	busy_energy = float(words[-1])

	line = file.readline()
	line = file.readline()
	words = line.split()

	idle_energy = float(words[-1])

if energyRatio=="1":
	print(nWifi, tx_energy/nbPackets, rx_energy/nbPackets, busy_energy/nbPackets, idle_energy/nbPackets, (tx_energy + rx_energy + busy_energy + idle_energy)/nbPackets) # Energy in Joules / Byte
else:
	print(nWifi, tx_energy/simulation_time, rx_energy/simulation_time, busy_energy/simulation_time, idle_energy/simulation_time, (tx_energy + rx_energy + busy_energy + idle_energy)/simulation_time) # Power in Watts