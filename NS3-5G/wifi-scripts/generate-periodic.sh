declare -a deviceModel="Soekris" # For energy computation
declare -a MCS=9
declare -a txPower=9 #dBm
declare -a trafficDirection="upstream" 
declare -a simulationTime="3600" # Seconds
declare -a payloadSize="35" # Bytes, for all the packet (+ 12 Header)
declare -a period="3600" # Seconds between packets
declare -a nWifi=1 # Number of end-devices

declare -a latency=0 # Latency in boxplots
declare -a energyPower=0 # Energy Power consumption of one device in Watts
declare -a energyRatio=1 # Energy Ratio consumption of one device in Joules/Byte
declare -a crossFactor=0 # Consider cross-factor or not
declare -a batteryRV=0 # Use the RV Model battery
declare -a successRate=0 # Packets (Layer 3) success rate percentage %

declare -a hiddenStations=0 # If hidden nodes are allowed or not (50%)

declare -a step=30 # Step between number of devices
declare -a string=""

declare -a nbRxBytes=0 # Number of received bytes by the receiver
declare -a nbTxPackets=0 # Number of sent packets by the sender
declare -a successRateValue=0 # Success rate percentagek
declare -a string="" # Buffer string

mkdir -p "telemetry-23Bytes"
path="telemetry-23Bytes"

source venv/bin/activate

for period in 360
do
    for payloadSize in 35
    do
        for nWifi in $(seq $1 $step $2)
        do    
            ./waf --run "scratch/script-periodic.cc --simulationTime=$simulationTime --trafficDirection=$trafficDirection --period=$period --payloadSize=$payloadSize --nWifi=$nWifi --MCS=$MCS --txPower=$txPower --energyPower=$energyPower --energyRatio=$energyRatio --crossFactor=$crossFactor --batteryRV=$batteryRV --latency=$latency --successRate=$successRate --hiddenStations=$hiddenStations" 2> "${path}/$nWifi-$period-$MCS-$payloadSize-log.txt" > "${path}/$nWifi-$period-$MCS-$payloadSize-out.txt"
            
            if [[ $latency -eq 1 ]]
            then
                mkdir -p "$path/latency"

                cat "${path}/$nWifi-$period-$MCS-$payloadSize-log.txt" | grep -e 'client sent 1023 bytes' -e 'server received 1023 bytes from' > "${path}/latency/$nWifi-$period-$MCS-$payloadSize-log-parsed.txt"
                python3 get_latencies.py "${path}/latency/$nWifi-$period-$MCS-$payloadSize-log-parsed.txt" $nWifi "${path}/latency/$nWifi.csv"
            fi

            if [[ $energyPower -eq 1 ]]
            then
                mkdir -p "$path/energy"

                if [[ $crossFactor -eq 1 ]]
                then
                    nbTxPackets=$(cat "${path}/$nWifi-$period-$MCS-$payloadSize-out.txt" | grep -e 'Number of generated packets by one station:' | awk 'NF>0{print $NF}')
                    
                    python3 get_energy_ns3_model.py "${path}/$nWifi-$period-$MCS-$payloadSize-log.txt" $simulationTime $nWifi 0 0 > "${path}/energy/cross-factor-buffer.txt"
                    python3 get_cross_factor.py "${path}/$nWifi-$period-$MCS-$payloadSize.tr" "${path}/energy/cross-factor-buffer.txt" $simulationTime $payloadSize $deviceModel 0 0 $nbTxPackets "${path}/energy/$period-$MCS-$payloadSize-power.csv"
                else
                    python3 get_energy_ns3_model.py "${path}/$nWifi-$period-$MCS-$payloadSize-log.txt" $simulationTime $nWifi 0 0 >> "${path}/energy/$period-$MCS-$payloadSize-power.csv"
                fi
            fi
            
            if [[ $energyRatio -eq 1 ]]
            then
                mkdir -p "$path/energy"

                nbRxBytes=$(cat "${path}/$nWifi-$period-$MCS-$payloadSize-out.txt" | grep -e 'Number of received bytes from one station:' | awk 'NF>0{print $NF}')

                if [[ $crossFactor -eq 1 ]]
                then
                    nbTxPackets=$(cat "${path}/$nWifi-$period-$MCS-$payloadSize-out.txt" | grep -e 'Number of generated packets by one station:' | awk 'NF>0{print $NF}')

                    python3 get_energy_ns3_model.py "${path}/$nWifi-$period-$MCS-$payloadSize-log.txt" $simulationTime $nWifi 1 $nbRxBytes > "${path}/energy/cross-factor-buffer.txt"
                    python3 get_cross_factor.py "${path}/$nWifi-$period-$MCS-$payloadSize.tr" "${path}/energy/cross-factor-buffer.txt" $simulationTime $payloadSize $deviceModel 1 $nbRxBytes $nbTxPackets "${path}/energy/$period-$MCS-$payloadSize-ratio.csv"
                else
                    python3 get_energy_ns3_model.py "${path}/$nWifi-$period-$MCS-$payloadSize-log.txt" $simulationTime $nWifi 1 $nbRxBytes >> "${path}/energy/$period-$MCS-$payloadSize-ratio.csv" 
                fi
            fi

            if [[ $successRate -eq 1 ]]
            then
                mkdir -p "$path/success-rate"

                successRateValue=$(cat "${path}/$nWifi-$period-$MCS-$payloadSize-out.txt" | grep -e 'Success rate:' | awk 'NF>0{print $NF}')

                echo $nWifi, $successRateValue >> "${path}/success-rate/$period-$MCS-$payloadSize.csv" 
            fi

            #rm "${path}/$nWifi-$period-$MCS-$payloadSize.tr" "${path}/$nWifi-$period-$MCS-$payloadSize.pcap" "${path}/$nWifi-$period-$MCS-$payloadSize-out.txt" "${path}/$nWifi-$period-$MCS-$payloadSize-log.txt" "${path}/latency/$nWifi-$period-$MCS-$payloadSize-log-parsed.txt"
        done

        if [[ $latency -eq 1 ]]
        then
            cd "${path}/latency/"
            python3 ../../merge.py $nWifi > "$period-$MCS-$payloadSize.csv"
            cd ../../

            for nWifi in $(seq $1 $step $2)
            do
                rm  "${path}/latency/$nWifi.csv"
            done
        fi
    done
done