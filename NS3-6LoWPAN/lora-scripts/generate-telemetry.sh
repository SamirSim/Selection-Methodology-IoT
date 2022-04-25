declare -a simulationTime=3600 # Seconds
declare -a payloadSize=23 # Bytes
declare -a period=1000 # Seconds
declare -a nSta=2 # Number of end-devices
declare -a distance=100 # Meters from ED to GW

declare -a energyRatio=1 # Energy Ratio consumption of one device in Joules/Byte
declare -a successRate=1 # Packets (Layer 3) success rate percentage %
declare -a batteryLife=0 # Battery Lifetime of one-device

declare -a nbPackets=0 
declare -a energyWatts=0
declare -a energyJoulesPerByte=0
declare -a successRateValue=0 # Success rate percentage

declare -a step=2000

path="telemetry-final"
mkdir -p "$path"

for period in 900
do
    for distance in 100
    do
        for nSta in $(seq $1 $step $2)
        do 
            ./waf --run "scratch/lora-telemetry.cc --simulationTime=$simulationTime --payloadSize=$payloadSize --period=$period --nSta=$nSta --distance=$distance --energyRatio=$energyRatio --successRate=$successRate --batteryLife=$batteryLife" 2> "${path}/$nSta-$period-$distance-$payloadSize-log.txt" > "${path}/$nSta-$period-$distance-$payloadSize-out.txt"
            
            if [[ $successRate -eq 1 ]]
            then
                mkdir -p "$path/success-rate"
                python3 get_loss.py "${path}/$nSta-$period-$distance-$payloadSize-out.txt" "${path}/success-rate/$period-$distance-$payloadSize.csv" $nSta
            fi 

            if [[ $energyRatio -eq 1 ]]
            then
                mkdir -p "$path/energy"
                energyJoules=$(cat "${path}/$nSta-$period-$distance-$payloadSize-log.txt" | grep -e "LoraRadioEnergyModel:Total energy consumption" | tail -1 | awk 'NF>1{print $NF}' | sed 's/J//g')
                nbPackets=$(cat "${path}/$nSta-$period-$distance-$payloadSize-out.txt" | tail -1 | awk '{print $2;}')
                nbBytes=$(bc <<< "($nbPackets/$nSta)*($payloadSize+9)")
                energyJoulesPerByte=$(bc <<< "scale=10;$energyJoules/$nbBytes" | awk '{printf "%f", $0}') # Joules/Byte
                #echo $nSta $energyJoulesPerByte >> "${path}/energy/$period-$distance-$payloadSize-ratio.csv" 
                echo $nSta $energyJoules $nbBytes $energyJoulesPerByte
            fi     
            #cat "${path}/$nSta-$period-$distance-$payloadSize-out.txt"
            rm "${path}/$nSta-$period-$distance-$payloadSize-out.txt" "${path}/$nSta-$period-$distance-$payloadSize-log.txt"
        done
    done
done