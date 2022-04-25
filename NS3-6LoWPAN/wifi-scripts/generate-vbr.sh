declare -a deviceModel="Soekris" # For energy computation
declare -a MCS=9
declare -a txPower=9 #dBm
declare -a trafficDirection="upstream"
declare -a simulationTime="500" # Seconds
declare -a payloadSize="1024" # Bytes
declare -a dataRate=2 # Mbps
declare -a nWifi=1 # Number of end-devices

declare -a latency=1 # Mean latency
declare -a energyPower=1 # Energy Power consumption of one device in Watts
declare -a energyRatio=1 # Energy Ratio consumption of one device in Joules/Byte
declare -a crossFactor=0 # Consider cross-factor or not
declare -a batteryRV=0 # Use the RV Model battery
declare -a successRate=1 # Packets (Layer 3) success rate percentage %

declare -a hiddenStations=0 # If hidden nodes are allowed or not (50%)

declare -a step=3 # Step between number of devices
declare -a string=""

declare -a nbRxBytes=0 # Number of received bytes by the receiver
declare -a consumedEnergy=0
declare -a nbTxPackets=0 # Number of sent packets by the sender
declare -a successRateValue=0 # Success rate percentage
declare -a string="" # Buffer string

mkdir -p "vbr"
path="vbr"

source venv/bin/activate

for dataRate in 2
do
    for MCS in 6
    do
        for nWifi in $(seq $1 $step $2)
        do    
            ./waf --run "scratch/script-vbr.cc --simulationTime=$simulationTime --trafficDirection=$trafficDirection --dataRate=$dataRate --payloadSize=$payloadSize --nWifi=$nWifi --MCS=$MCS --txPower=$txPower --energyPower=$energyPower --energyRatio=$energyRatio --crossFactor=$crossFactor --batteryRV=$batteryRV --latency=$latency --successRate=$successRate --hiddenStations=$hiddenStations" 2> "${path}/$nWifi-$dataRate-$MCS-$payloadSize-log.txt" > "${path}/$nWifi-$dataRate-$MCS-$payloadSize-out.txt"
            
            if [[ $latency -eq 1 ]]
            then
                mkdir -p "$path/latency"

                cat "${path}/$nWifi-$dataRate-$MCS-$payloadSize-log.txt" | grep -e 'client sent 1023 bytes' -e 'server received 1023 bytes from' > "${path}/latency/$nWifi-$dataRate-$MCS-$payloadSize-log-parsed.txt"
                python3 get_latencies.py "${path}/latency/$nWifi-$dataRate-$MCS-$payloadSize-log-parsed.txt" $nWifi "${path}/latency/$dataRate-$MCS-$payloadSize-$nWifi.csv"
            fi

            if [[ $energyPower -eq 1 ]]
            then
                mkdir -p "$path/energy"

                if [[ $crossFactor -eq 1 ]]
                then
                    nbTxPackets=$(cat "${path}/$nWifi-$dataRate-$MCS-$payloadSize-out.txt" | grep -e 'Number of generated packets by one station:' | awk 'NF>0{print $NF}')
                    
                    python3 get_energy_ns3_model.py "${path}/$nWifi-$dataRate-$MCS-$payloadSize-log.txt" $simulationTime $nWifi 0 0 > "${path}/energy/cross-factor-buffer.txt"
                    python3 get_cross_factor.py "${path}/$nWifi-$dataRate-$MCS-$payloadSize.tr" "${path}/energy/cross-factor-buffer.txt" $simulationTime $payloadSize $deviceModel 0 0 $nbTxPackets "${path}/energy/$dataRate-$MCS-$payloadSize-power.csv"
                else
                    python3 get_energy_ns3_model.py "${path}/$nWifi-$dataRate-$MCS-$payloadSize-log.txt" $simulationTime $nWifi 0 0 >> "${path}/energy/$dataRate-$MCS-$payloadSize-power.csv"
                fi
            fi
            
            if [[ $energyRatio -eq 1 ]]
            then
                mkdir -p "$path/energy"

                nbRxBytes=$(cat "${path}/$nWifi-$dataRate-$MCS-$payloadSize-out.txt" | grep -e 'Number of received bytes from one station:' | awk 'NF>0{print $NF}')

                if [[ $crossFactor -eq 1 ]]
                then
                    nbTxPackets=$(cat "${path}/$nWifi-$dataRate-$MCS-$payloadSize-out.txt" | grep -e 'Number of generated packets by one station:' | awk 'NF>0{print $NF}')

                    python3 get_energy_ns3_model.py "${path}/$nWifi-$dataRate-$MCS-$payloadSize-log.txt" $simulationTime $nWifi 1 $nbRxBytes > "${path}/energy/cross-factor-buffer.txt"
                    python3 get_cross_factor.py "${path}/$nWifi-$dataRate-$MCS-$payloadSize.tr" "${path}/energy/cross-factor-buffer.txt" $simulationTime $payloadSize $deviceModel 1 $nbRxBytes $nbTxPackets "${path}/energy/$dataRate-$MCS-$payloadSize-ratio.csv"
                else
                    python3 get_energy_ns3_model.py "${path}/$nWifi-$dataRate-$MCS-$payloadSize-log.txt" $simulationTime $nWifi 1 $nbRxBytes >> "${path}/energy/$dataRate-$MCS-$payloadSize-ratio.csv" 
                fi
            fi

            if [[ $batteryRV -eq 1 ]]
            then
                mkdir -p "$path/energy"

                consumedEnergy=$(cat "${path}/$nWifi-$dataRate-$MCS-$payloadSize-out.txt" | grep -e 'Total Consumed energy = ' | tail -1 | awk 'NF>0{print $NF}')
                    
                echo $consumedEnergy
                
            fi

            if [[ $successRate -eq 1 ]]
            then
                mkdir -p "$path/success-rate"

                successRateValue=$(cat "${path}/$nWifi-$dataRate-$MCS-$payloadSize-out.txt" | grep -e 'Success rate:' | awk 'NF>0{print $NF}')

                echo $nWifi, $successRateValue >> "${path}/success-rate/$dataRate-$MCS-$payloadSize.csv" 
            fi

            rm "${path}/$nWifi-$dataRate-$MCS-$payloadSize.tr" "${path}/$nWifi-$dataRate-$MCS-$payloadSize.pcap" "${path}/$nWifi-$dataRate-$MCS-$payloadSize-out.txt" "${path}/$nWifi-$dataRate-$MCS-$payloadSize-log.txt" "${path}/latency/$nWifi-$dataRate-$MCS-$payloadSize-log-parsed.txt"
        done

        if [[ $latency -eq 1 ]]
        then
            cd "${path}/latency/"
            python3 ../../merge.py $nWifi $dataRate $MCS $payloadSize > "$dataRate-$MCS-$payloadSize.csv"
            cd ../../

            for nWifi in $(seq $1 $step $2)
            do
                rm  "${path}/latency/$dataRate-$MCS-$payloadSize-$nWifi.csv"
            done
        fi
    done
done