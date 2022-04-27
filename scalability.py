import os

results = []

def explore(technology_configuration, scenario_parameters, simulation_time, use_case):
    configuration_parameters = technology_configuration.split("-")
    os.environ['DISTANCE'] = str(scenario_parameters['distance'])
    os.environ['NBDEVICES'] = str(scenario_parameters['nb_devices'])
    os.environ['TRAFFICDIR'] = scenario_parameters['traffic_direction']
    os.environ['PACKETSIZE'] = str(scenario_parameters['packet_size'])
    os.environ['PERIOD'] = str(scenario_parameters['packet_period'])
    os.environ['SIMULATION_TIME'] = str(simulation_time)
    os.environ['BATTERY_CAPACITY'] = str(scenario_parameters['battery_capacity'])
    os.environ['BATTERY_VOLTAGE'] = str(scenario_parameters['battery_voltage'])
    os.environ['MOBILENODES'] = str(scenario_parameters['mobile_nodes'])
    os.environ['PROPLOSS'] = str(scenario_parameters['propagation_loss_model'])
    technology = configuration_parameters[0]

    if technology == "WiFi":
        mcs = configuration_parameters[1]
        sgi = configuration_parameters[2]
        spatial_stream = configuration_parameters[3]
        agregation = configuration_parameters[4]
        ngateway = configuration_parameters[5]

        for nb_devices in range(int(scenario_parameters['nb_devices'])+10, int(scenario_parameters['nb_devices'])+50):
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


    elif technology == "LoRaWAN":
        sf = configuration_parameters[1]
        coding_rate = configuration_parameters[2]
        crc = configuration_parameters[3]
        traffic = configuration_parameters[4]
        ngateway = configuration_parameters[5]

    elif technology == "6LoWPAN":
        frame_retries = configuration_parameters[1]
        csma_backoffs = configuration_parameters[2]
        max_be = configuration_parameters[3]
        min_be = configuration_parameters[4]
        ngateway = configuration_parameters[5]

    elif technology == "802.11ah":
        mcs = configuration_parameters[1]
        sgi = configuration_parameters[2]
        beacon_interval = configuration_parameters[3]
        nraw_groups = configuration_parameters[4]
        ngateway = configuration_parameters[5]
    