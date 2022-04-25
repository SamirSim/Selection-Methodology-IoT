from math import sqrt, dist
import os, threading, subprocess, time

scenario_parameters = {
    "distance": 1,
    "nb_devices": 3,
    "traffic_direction": "upstream",
    "packet_size": 200,
    "packet_period": 2,
    "battery_capacity": 5200,
    "battery_voltage": 12
}

simulation_time = 36

""""
metrics_weights = {
    "success_rate":0.7,
    "battery_lifetime":0.3,
    "latency":0,
    "throughput":0,
    "energy_consumption":0
}
"""
# Weights of attributes (positive and sum = 1)
metrics_weights = [0.2, 0.2, 0.2, 0.0, 0.2]

# Indicate type of metric (1 for benefit and 0 for cost)
I = [1, 1, 0, 1, 0]

result = [0, 0, 0, 0, 0]

technologies = ["6LoWPAN", "Wi-Fi", "LoRaWAN", "802.11ah"]

evaluation_tool = "Simulation"

def pruning(scenario_parameters, technologies):
    pass

def evaluation(scenario_parameters, evaluation_tool, technology):
    os.environ['DISTANCE'] = str(scenario_parameters['distance'])
    os.environ['NBDEVICES'] = str(scenario_parameters['nb_devices'])
    os.environ['TRAFFICDIR'] = scenario_parameters['traffic_direction']
    os.environ['PACKETSIZE'] = str(scenario_parameters['packet_size'])
    os.environ['PERIOD'] = str(scenario_parameters['packet_period'])
    os.environ['SIMULATION_TIME'] = str(simulation_time)
    os.environ['BATTERY_CAPACITY'] = str(scenario_parameters['battery_capacity'])
    os.environ['BATTERY_VOLTAGE'] = str(scenario_parameters['battery_voltage'])

    output = ""
    
    if technology == "Wi-Fi":
        try:
            output = subprocess.check_output('./waf --run "scratch/wifi-periodic.cc --distance=$DISTANCE --simulationTime=$SIMULATION_TIME --nWifi=$NBDEVICES --trafficDirection=$TRAFFICDIR --period=$PERIOD --payloadSize=$PACKETSIZE" 2> log-wifi.txt', shell=True, text=True,stderr=subprocess.DEVNULL)
            latency = subprocess.check_output('cat "log-wifi.txt" | grep -e "client sent 1023 bytes" -e "server received 1023 bytes from" > "log-wifi-parsed.txt"; python3 wifi-scripts/get_latencies.py "log-wifi-parsed.txt"', shell=True, text=True,stderr=subprocess.DEVNULL)
            #subprocess.check_output('rm "log-wifi.txt"; rm "log-wifi-parsed.txt"', shell=True, text=True,stderr=subprocess.DEVNULL)

            lines = output.splitlines()
            line = lines[0]
            i = 0
            while "Total" not in line:
                i = i + 1
                line = lines[i]
            energy = float(lines[i].split()[-1])
            battery_lifetime = float(lines[i+1].split()[-1])
            throughput = float(lines[i+2].split()[-1])
            success_rate = float(lines[i+3].split()[-1])
            latency = float(latency) * 1000

            result = [success_rate, battery_lifetime, latency, throughput, energy]
        except:
            result = [0, 0, 0, 0, 0]
        
    elif technology == "LoRaWAN":
        try:
            output = subprocess.check_output('./waf --run "scratch/lora-periodic.cc --distance=$DISTANCE --simulationTime=$SIMULATION_TIME --nSta=$NBDEVICES --payloadSize=$PACKETSIZE --period=$PERIOD" 2> log-lora.txt', shell=True, text=True,stderr=subprocess.DEVNULL)
            output = output + "Energy consumption: " + subprocess.check_output("cat 'log-lora.txt' | grep -e 'LoraRadioEnergyModel:Total energy consumption' | tail -1 | awk 'NF>1{print $NF}' | sed 's/J//g'", shell=True, text=True,stderr=subprocess.DEVNULL)
            latency = subprocess.check_output('cat "log-lora.txt" | grep -e "GatewayLorawanMac:Receive()" -e "EndDeviceLorawanMac:Send(" > "log-lora-parsed.txt"; python3 lora-scripts/get_latencies.py "log-lora-parsed.txt"', shell=True, text=True,stderr=subprocess.DEVNULL)
            #subprocess.check_output('rm "log-lora.txt"; rm "log-lora-parsed.txt"', shell=True, text=True,stderr=subprocess.DEVNULL)

            throughput = 0
            lines = output.splitlines()
            line = lines[0]
            i = 0
            while "Success" not in line:
                i = i + 1
                line = lines[i]
            success_rate = round(float(lines[i].split()[-1]), 2)
            throughput = round(float(lines[i+1].split()[-1]), 2)
            energy = float(lines[i+2].split()[-1])
            capacity = (float(scenario_parameters['battery_capacity']) / 1000.0) * float(scenario_parameters['battery_voltage']) * 3600
            battery_lifetime = round(((capacity / energy) * float(simulation_time)) / 86400, 2)
            energy = round(energy, 2)
            latency = float(latency) * 1000

            result = [success_rate, battery_lifetime, latency, throughput, energy]
        except:
            result = [0, 0, 0, 0, 0]

    elif technology == "6LoWPAN":
        try:
            output = subprocess.check_output('./waf --run "scratch/6lowpan-periodic.cc --distance=$DISTANCE --simulationTime=$SIMULATION_TIME --nDevices=$NBDEVICES --packetSize=$PACKETSIZE --period=$PERIOD --capacity=$BATTERY_CAPACITY --voltage=$BATTERY_VOLTAGE" 2> log-6lowpan.txt', shell=True, text=True,stderr=subprocess.DEVNULL)

            latency = subprocess.check_output('cat "log-6lowpan.txt" | grep -e "client sent 50 bytes" -e "server received 50 bytes from" > "log-6lowpan-parsed.txt"; python3 wifi-scripts/get_latencies.py "log-6lowpan-parsed.txt"', shell=True, text=True,stderr=subprocess.DEVNULL)
            #subprocess.check_output('rm "log-6lowpan.txt"; rm "log-6lowpan-parsed.txt"', shell=True, text=True,stderr=subprocess.DEVNULL)

            lines = output.splitlines()
            line = lines[0]
            i = 0
            while "Total" not in line:
                i = i + 1
                line = lines[i]
            energy = float(lines[i].split()[-1])
            battery_lifetime = float(lines[i+1].split()[-1])
            throughput = float(lines[i+2].split()[-1])
            success_rate = float(lines[i+3].split()[-1])
            latency = float(latency) * 1000

            result = [success_rate, battery_lifetime, latency, throughput, energy]
        except Exception as e:
            print(e)
            result = [0, 0, 0, 0, 0]
    
    print(technology, result)
    return result

def normalize(attributes):
    normalized = []
    for i in range(len(attributes)):
        metrics = []
        for j in range(len(attributes[0])):
            metrics.append(attributes[i][j] / sqrt(sum(attributes[m][j]**2 for m in range(len(attributes)))))
        normalized.append(metrics)
        
    return normalized

def weight(attributes, metrics_weights):
    weighted = []
    for i in range(len(attributes)):
        metrics = []
        for j in range(len(attributes[0])):
            metrics.append(metrics_weights[j] * attributes[i][j])
        weighted.append(metrics)

    return weighted

def selection(attributes, I):
    best_solution = []
    worst_solution = []
    for j in range(len(attributes[0])):
        best_value = attributes[0][j]
        worst_value = attributes[0][j]
        for i in range(len(attributes)):
            if I[j] == 1:
                best_value = max(best_value, attributes[i][j])
                worst_value = min(worst_value, attributes[i][j])
            else:
                best_value = min(best_value, attributes[i][j])
                worst_value = max(worst_value, attributes[i][j])
        best_solution.append(best_value)
        worst_solution.append(worst_value)

    distances_from_best = []
    distances_from_worst = []
    for i in range(len(attributes)):
        distances_from_best.append(dist(attributes[i], best_solution))
        distances_from_worst.append(dist(attributes[i], worst_solution))

    coefficients = []
    for i in range(len(attributes)):
        coefficient = distances_from_worst[i] / (distances_from_worst[i] + distances_from_best[i])
        coefficients.append(coefficient)

    print(coefficients)
    return coefficients

attributes = []

for technology in technologies:
    attributes.append(evaluation(scenario_parameters, evaluation_tool, technology))

attributes.append([99.86, 1154.56, 3, 0.58, 8.1])

result = selection(weight(normalize(attributes), metrics_weights), I)
print("Selection: The best technology is: ", technologies[result.index(max(result))])