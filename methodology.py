from ctypes import resize
from math import sqrt, dist, pi
import os, threading, subprocess, time
from shutil import ExecError
import sys
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
import pandas as pd
import random

constraints = 0

def price_solution(technology, ned, ngw, battery_lifetime, use_case):
    battery_changing_price = 500
    
    ed_wifi_price = 10
    ed_lorawan_price = 5
    ed_6lowpan_price = 30
    ed_halow_price = 15

    gw_wifi_price = 100
    gw_lorawan_price = 1000
    gw_6lowpan_price = 200
    gw_halow_price = 1000

    if use_case == "surveillance":
        period = 60
    elif use_case == "building-automation":
        period = 830
    elif use_case == "smart-grid":
        period = 3650
    else:
        period = 100
    
    if technology == "WiFi":
        return ((gw_wifi_price * ngw) + (ed_wifi_price * ned) + ((int(period / battery_lifetime)) * battery_changing_price))
    if technology == "LoRaWAN":
        return ((gw_lorawan_price * ngw) + (ed_lorawan_price * ned) + ((int(period / battery_lifetime)) * battery_changing_price))
    if technology == "6LoWPAN":
        return ((gw_6lowpan_price * ngw) + (ed_6lowpan_price * ned) + ((int(period / battery_lifetime)) * battery_changing_price))
    if technology == "802.11ah":
        return ((gw_halow_price * ngw) + (ed_halow_price * ned) + ((int(period / battery_lifetime)) * battery_changing_price))


def strictly_continuous(value):
    return value

def continuous_constraint(value, type, constraint): # type == 1 <=> the more the better, else otherwise
    if type == 1:
        if value < constraint:
            return 0
        else:
            return value
    else:
        if value < constraint:
            return constraint
        else:
            return value

use_case = sys.argv[1]

if use_case == "surveillance":
    max_latency = 10
    min_lifetime = 7
elif use_case == "building-automation":
    max_latency = 100
    min_lifetime = 90
elif use_case == "smart-grid":
    max_latency = 1000
    min_lifetime = 365
else:
    max_latency = 1
    min_lifetime = 0

scenario_parameters = {
    "distance": 20,
    "nb_devices": 10,
    "traffic_direction": "upstream",
    "packet_size": 30,
    "packet_period": 1,
    "battery_capacity": 2400,
    "battery_voltage": 3
}

simulation_time = 10
propagation_loss_model = "LogDistancePropagationLossModel"

print(scenario_parameters, propagation_loss_model)

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
metrics_weights = [0.2, 0.2, 0.2, 0.2]

# Indicate type of metric (1 for benefit and 0 for cost)
I = [1, 1, 0, 0]

result = [0, 0, 0, 0]
results = []
price = 0


technologies = ["WiFi", "802.11ah", "LoRaWAN", "6LoWPAN"]

technologies_configuration = []

evaluation_tool = "Simulation"

number_gateways = [1, 2, 3]

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
    os.environ['NBGW'] = "2"
    os.environ['PROPLOSS'] = propagation_loss_model

    #output = subprocess.check_output('python3 generate-positions.py $DISTANCE $NBDEVICES $NBGW $FILENAME', shell=True, text=True,stderr=subprocess.DEVNULL)

    output = ""
    
    if technology == "WiFi":
        try:
            if scenario_parameters['distance'] > 50:
                print("pruning applied on ", technology)
            else:
                MCS_list = ["-1"]
                SGI_list = [0]
                spatial_streams_list = [1]
                agregation_list = [0]
                for mcs in MCS_list:
                    for sgi in SGI_list:
                        for spatial_stream in spatial_streams_list:
                            for agregation in agregation_list:
                                for ngateway in number_gateways:
                                    os.environ['MCS'] = str(mcs)
                                    os.environ['SGI'] = str(sgi)
                                    os.environ['SPATIALSTREAMS'] = str(spatial_stream)
                                    os.environ['AGREGATION'] = str(agregation)
                                    os.environ['NGW'] = str(ngateway)
                                    os.environ['LOGFILE'] = "log-"+technology+"-"+str(use_case)+".txt"
                                    os.environ['LOGFILEPARSED'] = "log-"+technology+"-"+str(use_case)+"-parsed.txt"

                                    output = subprocess.check_output('cd NS3-6LoWPAN; ./waf --run "scratch/wifi-periodic.cc --propLoss=$PROPLOSS --nGW=$NGW --agregation=$AGREGATION --sgi=$SGI --spatialStreams=$SPATIALSTREAMS --simulationTime=$SIMULATION_TIME --nWifi=$NBDEVICES --trafficDirection=$TRAFFICDIR --period=$PERIOD --payloadSize=$PACKETSIZE --batteryCap=$BATTERY_CAPACITY --voltage=$BATTERY_VOLTAGE" 2> $LOGFILE', shell=True, text=True,stderr=subprocess.DEVNULL)
                                    latency = subprocess.check_output('cd NS3-6LoWPAN; cat $LOGFILE | grep -e "client sent 1023 bytes" -e "server received 1023 bytes from" > $LOGFILEPARSED; python3 wifi-scripts/get_latencies.py $LOGFILEPARSED', shell=True, text=True,stderr=subprocess.DEVNULL)
                                    #subprocess.check_output('rm "log-wifi.txt"; rm "log-wifi-parsed.txt"', shell=True, text=True,stderr=subprocess.DEVNULL)

                                    lines = output.splitlines()
                                    line = lines[0]
                                    #òprint(lines)
                                    i = 0
                                    while "Total" not in line:
                                        i = i + 1
                                        line = lines[i]
                                    energy = float(lines[i].split()[-1])
                                    battery_lifetime = float(lines[i+1].split()[-1])
                                    #throughput = float(lines[i+2].split()[-1])
                                    success_rate = float(lines[i+3].split()[-1])
                                    latency = float(latency) * 1000
                                    price = price_solution(technology, scenario_parameters['nb_devices'], ngateway, battery_lifetime, use_case)

                                    if constraints:
                                        battery_lifetime = continuous_constraint(battery_lifetime, 1, min_lifetime)
                                        latency = continuous_constraint(latency, 0, max_latency)

                                    result = [success_rate, battery_lifetime, latency, price]
                                    
                                    #configuration = "WiFi-"+str(mcs)+"-"+str(sgi)+"-"+str(spatial_stream)+"-"+str(agregation)+"-"+str(ngateway)
                                    configuration = "WiFi-GW"+str(ngateway)
                                    technologies_configuration.append(configuration)

                                    #print("WiFi: ", mcs, sgi, spatial_stream, agregation, ngateway, result)
                                    results.append(result)
        except Exception as e:
            print(e)
            result = [0, 0, 0, 0]
        
    elif technology == "LoRaWAN":
        try:
            if scenario_parameters['packet_size'] > 100 or scenario_parameters['packet_period'] < 30:
                print("pruning applied on ", technology)
            else:
                SF_list = [0]
                coding_rate_list = [1]
                crc_list = [0] # Cyclic redundancy check
                traffic_list = ["Unconfirmed"] # Confirmed or unconfirmed

                for sf in SF_list:
                    for coding_rate in coding_rate_list:
                        for crc in crc_list:
                            for traffic in traffic_list:
                                for ngateway in number_gateways:
                                    os.environ['SF'] = str(sf)
                                    os.environ['CODINGRATE'] = str(coding_rate)
                                    os.environ['CRC'] = str(crc)
                                    os.environ['TRAFFIC'] = str(traffic)
                                    os.environ['NGW'] = str(ngateway)
                                    os.environ['LOGFILE'] = "log-"+technology+"-"+str(use_case)+".txt"
                                    os.environ['LOGFILEPARSED'] = "log-"+technology+"-"+str(use_case)+"-parsed.txt"

                                    output = subprocess.check_output('cd NS3-6LoWPAN; ./waf --run "scratch/lora-periodic.cc --propLoss=$PROPLOSS --nGateways=$NGW --trafficType=$TRAFFIC --SF=$SF --codingRate=$CODINGRATE --crc=$CRC --distance=$DISTANCE --simulationTime=$SIMULATION_TIME --nSta=$NBDEVICES --payloadSize=$PACKETSIZE --period=$PERIOD" 2> $LOGFILE', shell=True, text=True,stderr=subprocess.DEVNULL)
                                    output = output + "Energy consumption: " + subprocess.check_output("cd NS3-6LoWPAN; cat $LOGFILE | grep -e 'LoraRadioEnergyModel:Total energy consumption' | tail -1 | awk 'NF>1{print $NF}' | sed 's/J//g'", shell=True, text=True,stderr=subprocess.DEVNULL)
                                    latency = subprocess.check_output('cd NS3-6LoWPAN; cat $LOGFILE | grep -e "Total time" > $LOGFILEPARSED; python3 lora-scripts/get_latencies.py $LOGFILEPARSED', shell=True, text=True,stderr=subprocess.DEVNULL)
                                    #subprocess.check_output('rm "log-lora.txt"; rm "log-lora-parsed.txt"', shell=True, text=True,stderr=subprocess.DEVNULL)

                                    #throughput = 0
                                    lines = output.splitlines()
                                    line = lines[0]
                                    i = 0
                                    while "Success" not in line:
                                        i = i + 1
                                        line = lines[i]
                                    success_rate = round(float(lines[i].split()[-1]), 2)
                                    #throughput = round(float(lines[i+1].split()[-1]), 2)
                                    energy = float(lines[i+2].split()[-1])
                                    capacity = (float(scenario_parameters['battery_capacity']) / 1000.0) * float(scenario_parameters['battery_voltage']) * 3600
                                    battery_lifetime = round(((capacity / energy) * float(simulation_time)) / 86400, 2)
                                    energy = round(energy, 2)
                                    latency = float(latency) * 1000
                                    price = price_solution(technology, scenario_parameters['nb_devices'], ngateway, battery_lifetime, use_case)

                                    if constraints:
                                        battery_lifetime = continuous_constraint(battery_lifetime, 1, min_lifetime)
                                        latency = continuous_constraint(latency, 0, max_latency)

                                    result = [success_rate, battery_lifetime, latency, price]

                                    #configuration = "LoRaWAN-"+str(sf)+"-"+str(coding_rate)+"-"+str(crc)+"-"+str(traffic)+"-"+str(ngateway)
                                    configuration = "LoRaWAN-GW"+str(ngateway)
                                    technologies_configuration.append(configuration)

                                    #print("LoRaWAN: ", sf, coding_rate, crc, traffic, ngateway, result)
                                    results.append(result)
        except Exception as e:
            print(e)
            result = [0, 0, 0, 0]

    elif technology == "6LoWPAN":
        try:
            if scenario_parameters['distance'] > 50 or scenario_parameters['packet_period'] < 0.1:
                print("pruning applied on ", technology)
            else:
                frame_retries_list = [4]
                csma_backoff_list = [5]
                max_be_list = [3]
                min_be_list = [4]

                for frame_retries in frame_retries_list:
                    for csma_backoff in csma_backoff_list:
                        for max_be in max_be_list:
                            for min_be in min_be_list:
                                for ngateway in number_gateways:
                                    os.environ['MAXFRAMERETRIES'] = str(frame_retries)
                                    os.environ['CSMABACKOFF'] = str(csma_backoff)
                                    os.environ['MAXBE'] = str(max_be)
                                    os.environ['MINBE'] = str(min_be)
                                    os.environ['NGW'] = str(ngateway)
                                    os.environ['LOGFILE'] = "log-"+technology+"-"+str(use_case)+".txt"
                                    os.environ['LOGFILEPARSED'] = "log-"+technology+"-"+str(use_case)+"-parsed.txt"

                                    output = subprocess.check_output('cd NS3-6LoWPAN; ./waf --run "scratch/6lowpan-periodic.cc --propLoss=$PROPLOSS --nGW=$NGW --maxFrameRetries=$MAXFRAMERETRIES --max_BE=$MAXBE --min_BE=$MINBE --csma_backoffs=$CSMABACKOFF --distance=$DISTANCE --simulationTime=$SIMULATION_TIME --nSta=$NBDEVICES --packetSize=$PACKETSIZE --period=$PERIOD --capacity=$BATTERY_CAPACITY --voltage=$BATTERY_VOLTAGE" 2> $LOGFILE', shell=True, text=True,stderr=subprocess.DEVNULL)

                                    latency = subprocess.check_output('cd NS3-6LoWPAN; cat $LOGFILE | grep -e "client sent 50 bytes" -e "server received 50 bytes from" > $LOGFILEPARSED; python3 wifi-scripts/get_latencies.py $LOGFILEPARSED', shell=True, text=True,stderr=subprocess.DEVNULL)
                                    #subprocess.check_output('rm "log-6lowpan.txt"; rm "log-6lowpan-parsed.txt"', shell=True, text=True,stderr=subprocess.DEVNULL)

                                    lines = output.splitlines()
                                    line = lines[0]
                                    i = 0
                                    while "Total" not in line:
                                        i = i + 1
                                        line = lines[i]
                                    energy = float(lines[i].split()[-1])
                                    battery_lifetime = float(lines[i+1].split()[-1])
                                    #throughput = float(lines[i+2].split()[-1])
                                    success_rate = float(lines[i+3].split()[-1])
                                    latency = float(latency) * 1000
                                    price = price_solution(technology, scenario_parameters['nb_devices'], ngateway, battery_lifetime, use_case)

                                    if constraints:
                                        battery_lifetime = continuous_constraint(battery_lifetime, 1, min_lifetime)
                                        latency = continuous_constraint(latency, 0, max_latency)

                                    result = [success_rate, battery_lifetime, latency, price]

                                    #configuration = "6LoWPAN-"+str(frame_retries)+"-"+str(csma_backoff)+"-"+str(max_be)+"-"+str(min_be)+"-"+str(ngateway)
                                    configuration = "6LoWPAN-GW"+str(ngateway)
                                    technologies_configuration.append(configuration)
                                    
                                    #print("6LoWPAN: ", frame_retries, csma_backoff, max_be, min_be, ngateway, result)
                                    results.append(result)
        except Exception as e:
            print(e)
            result = [0, 0, 0, 0]
    
    elif technology == "802.11ah":
        try:
            if scenario_parameters['distance'] > 2000 or scenario_parameters['packet_period'] < 0.1:
                print("pruning applied on ", technology)
            else:
                MCS_list = [0]
                SGI_list = [1]
                beacon_interval_list = [51200]
                nraw_groups_list = [1]

                for mcs in MCS_list:
                    for sgi in SGI_list:
                        for beacon_interval in beacon_interval_list:
                            for nraw_groups in nraw_groups_list:
                                for ngateway in number_gateways:
                                    os.environ['MCS'] = str(mcs)
                                    os.environ['SGI'] = str(sgi)
                                    os.environ['BEACONINTERVAL'] = str(beacon_interval)
                                    os.environ['NRAWGROUPS'] = str(nraw_groups)
                                    os.environ['NGW'] = str(ngateway)
                                    os.environ['LOGFILE'] = "log-"+technology+"-"+str(use_case)+".txt"

                                    #subprocess.check_output("cd NS3-802.11ah; ./scratch/RAWGenerate.sh $NBDEVICES $NRAWGROUPS $BEACONINTERVAL", shell=True, text=True,stderr=subprocess.DEVNULL)
                                    output = subprocess.check_output('cd NS3-802.11ah; ./waf --run "rca --Nsta=$NBDEVICES --propLoss=$PROPLOSS --nGW=$NGW --mcs=$MCS --sgi=$SGI --BeaconInterval=$BEACONINTERVAL --NGroup=$NRAWGROUPS --rho=$DISTANCE --simulationTime=$SIMULATION_TIME --trafficInterval=$PERIOD --payloadSize=$PACKETSIZE --batteryCapacity=$BATTERY_CAPACITY --voltage=$BATTERY_VOLTAGE" 2> $LOGFILE', shell=True, text=True,stderr=subprocess.DEVNULL)
                                    latency = subprocess.check_output('cd NS3-802.11ah; python3 halow-scripts/get_latencies.py $LOGFILE', shell=True, text=True,stderr=subprocess.DEVNULL)
                                    #subprocess.check_output('rm "log-wifi.txt"; rm "log-wifi-parsed.txt"', shell=True, text=True,stderr=subprocess.DEVNULL)

                                    lines = output.splitlines()
                                    line = lines[0]
                                    i = 0
                                    while "Total" not in line:
                                        i = i + 1
                                        line = lines[i]
                                    energy = float(lines[i].split()[-1])
                                    battery_lifetime = float(lines[i+1].split()[-1])
                                    #throughput = float(lines[i+2].split()[-1])
                                    success_rate = float(lines[i+3].split()[-1])
                                    latency = float(latency) * 1000
                                    price = price_solution(technology, scenario_parameters['nb_devices'], ngateway, battery_lifetime, use_case)

                                    if constraints:
                                        battery_lifetime = continuous_constraint(battery_lifetime, 1, min_lifetime)
                                        latency = continuous_constraint(latency, 0, max_latency)

                                    result = [success_rate, battery_lifetime, latency, price]

                                    #configuration = "802.11ah-"+str(mcs)+"-"+str(sgi)+"-"+str(beacon_interval)+"-"+str(nraw_groups)+"-"+str(ngateway)
                                    configuration = "HaLow-GW"+str(ngateway)
                                    technologies_configuration.append(configuration)

                                    #print("802.11ah: ", mcs, sgi, beacon_interval, nraw_groups, ngateway, result)
                                    results.append(result)
        except Exception as e:
            print(e)
            result = [0, 0, 0, 0]
    
    #print(technology, result)
    #return results

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
        if (distances_from_worst[i] + distances_from_best[i] == 0):
            coefficient = 1
        else:
            coefficient = distances_from_worst[i] / (distances_from_worst[i] + distances_from_best[i])
        coefficients.append(coefficient)

    #print("Coefficients ", coefficients)
    return coefficients

attributes = []

for technology in technologies:
    evaluation(scenario_parameters, evaluation_tool, technology)

#results = [ [100, 182.48, 0.34, 700], [100, 187.36, 0.21, 1400], [100, 188.9, 0.25, 2100], [81.9231, 804.818, 43.96, 200], [96.859, 1054.89, 39.86, 400], [99.902, 1208.81, 7.08, 600],
            #[90.9, 969.665, 11.63, 200], [100, 1052.57, 20.28, 400], [100, 1053.8, 33.35, 600]]

normalized = normalize(results)
weighted = weight(normalized, metrics_weights)

result = selection(weighted, I)

for i in range(len(result)):
    print(technologies_configuration[i], results[i], "{:.2f}".format(result[i]))

max_delivery = 0
max_lifetime = 0
max_latency = 0
max_cost = 0

for i in range(len(results)):
    max_delivery = max(max_delivery, results[i][0])
    max_lifetime = max(max_lifetime, results[i][1])
    max_latency = max(max_latency, results[i][2])
    max_cost = max(max_cost, results[i][3])

print("Selection: The best technology is: ", technologies_configuration[result.index(max(result))])

results = list(map(list, zip(*normalized)))

metrics = ['Packet Delivery \n (%)','Battery Lifetime \n (Days)','Packet Latency \n (ms)','Cost \n ($)']

df = pd.DataFrame({'group': technologies_configuration})

dictionary = {'group': technologies_configuration}
i = 0
for elem in results:
    dictionary[str(metrics[i])] = elem
    i = i + 1

df = pd.DataFrame(dictionary)

print(df)

deliveries = df['Packet Delivery \n (%)']
max_delivery_norm = max(deliveries)
lifetimes = df['Battery Lifetime \n (Days)']
max_lifetime_norm = max(lifetimes)
latencies = df['Packet Latency \n (ms)']
max_latency_norm = max(latencies)
costs = df['Cost \n ($)']
max_cost_norm = max(costs)

max_value = max(max_delivery_norm, max_cost_norm, max_lifetime_norm, max_latency_norm) + 0.07

"""
df = pd.DataFrame({
'group': ['A','B','C','D'],
'var1': [38, 1.5, 30, 4],
'var2': [29, 10, 9, 34],
'var3': [8, 39, 23, 24],
'var4': [7, 31, 33, 14],
'var5': [28, 15, 32, 14]
})
"""
# ------- PART 1: Create background
 
# number of variable
categories=list(df)[1:]
N = len(categories)
 
# What will be the angle of each axis in the plot? (we divide the plot / number of variable)
angles = [n / float(N) * 2 * pi for n in range(N)]
angles += angles[:1]
 
# Initialise the spider plot
ax = plt.subplot(111, polar=True)

# Remove spines
ax.spines["start"].set_color("none")
ax.spines["polar"].set_color("none")

GREY70 = "#b3b3b3"
GREY_LIGHT = "#f2efe8"
GREY50 = "#007A87"
# Angle values going from 0 to 2*pi
HANGLES = np.linspace(0, 2 * np.pi)

# Used for the equivalent of horizontal lines in cartesian coordinates plots 
# The last one is also used to add a fill which acts a background color.
H0 = np.zeros(len(HANGLES))
H1 = np.ones(len(HANGLES)) * max_value
H2 = np.ones(len(HANGLES))

# Add custom lines for radial axis (y) at 0, 0.5 and 1.
#ax.plot(HANGLES, H0, ls=(0, (6, 6)), c=GREY70)
ax.plot(HANGLES, H1, ls=(0, (6, 6)), c=GREY70)
#ax.plot(HANGLES, H2, ls=(0, (6, 6)), c=GREY70)

# Now fill the area of the circle with radius 1.
# This create the effect of gray background.
ax.fill(HANGLES, H1, GREY_LIGHT)

ax.plot([0, 0], [0, max_value], lw=2, c=GREY70)
ax.plot([np.pi, np.pi], [0, max_value], lw=2, c=GREY70)
ax.plot([np.pi / 2, np.pi / 2], [0, max_value], lw=2, c=GREY70)
ax.plot([-np.pi / 2, -np.pi / 2], [0, max_value], lw=2, c=GREY70)
 
# If you want the first axis to be on top:
ax.set_theta_offset(pi / 2)
ax.set_theta_direction(-1)
 
# Draw one axe per variable + add labels
ax.set_xticks(angles[:-1])
ax.set_xticklabels(categories)

# Remove lines for radial axis (y)
ax.set_yticks([])
ax.yaxis.grid(False)
ax.xaxis.grid(False)
 
# Draw ylabels
ax.set_rlabel_position(0)

#colors = ["#"+''.join([random.choice('0123456789ABCDEF') for j in range(6)])
 #            for i in range(len(technologies_configuration)//max(number_gateways))]

colors = ['#449e48', '#92623a', '#ed4d4d', '#26abff']

colorsDict = {
    'WiFi': '#449e48',
    '6LoWPAN': '#92623a',
    'HaLow': '#ed4d4d',
    'LoRaWAN': '#26abff'
}

handles = [
    Line2D(
        [], [], 
        c=color, 
        lw=3, 
        marker="o", 
        markersize=8, 
        label=species
    )
    for species, color in zip(categories, colors)
]
 
legend = ax.legend(
    handles=handles,
    loc=(1, 0),       # bottom-right
    labelspacing=1.5, # add space between labels
    frameon=False     # don't put a frame
)

X_VERTICAL_TICK_PADDING = 2
X_HORIZONTAL_TICK_PADDING = 35

# Adjust tick label positions ------------------------------------
XTICKS = ax.xaxis.get_major_ticks()
for tick in XTICKS[0::2]:
    tick.set_pad(X_VERTICAL_TICK_PADDING)
    
for tick in XTICKS[1::2]:
    tick.set_pad(X_HORIZONTAL_TICK_PADDING)

def cart2pol(x, y):
    rho = np.sqrt(x**2 + y**2)
    phi = np.arctan2(y, x)
    return(rho, phi)

# Add levels -----------------------------------------------------
# These labels indicate the values of the radial axis
#ax.text(max_delivery_norm_polar_x, max_delivery_norm_polar_y, str(max_delivery))
#ax.text(max_latency_norm_polar_x, max_latency_norm_polar_y, str(max_latency))
#ax.text(max_cost_norm_polar_x, max_cost_norm_polar_y, str(max_cost))
#ax.text(max_lifetime_norm_polar_x, max_lifetime_norm_polar_y, str(max_lifetime))
ax.text(np.deg2rad(5), max_delivery_norm, int(max_delivery))
ax.text(np.deg2rad(175), max_latency_norm, int(max_latency))
ax.text(np.deg2rad(85), max_lifetime_norm, int(max_lifetime))
ax.text(np.deg2rad(275), max_cost_norm + 0.05, int(max_cost))
#ax.text(-0.4, 0.5 + PAD, "")
#ax.text(-0.4, 1 + PAD, "")

# ------- PART 2: Add plots

linestyles = [ ":", "--", "-", "dotted", "solid", "dashed", "dashdot", "-."] 
 
# Plot each individual = each line of the data
# I don't make a loop, because plotting more than 3 groups makes the chart unreadableins
 
for i in range(len(technologies_configuration)):
    values=df.loc[i].drop('group').values.flatten().tolist()
    values += values[:1]
    color = colorsDict[technologies_configuration[i].split('-')[0]]
    ax.plot(angles, values, linewidth=1, color=color, linestyle=linestyles[i%max(number_gateways)], label=technologies_configuration[i])
    ax.fill(angles, values, alpha=0.1)

# Add legend
plt.legend(loc=(0.98,0.62), prop={'size': 8.5})

# Show the graph

plt.savefig(str(use_case)+'.png')