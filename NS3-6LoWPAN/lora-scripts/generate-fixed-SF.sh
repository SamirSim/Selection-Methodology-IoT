declare -a simulationTime=36000
declare -a nbPackets=0
declare -a payloadSize=23
declare -a energyJoules=0

i=$1
for SF in 7
do
    echo $SF
    #rm "${path}/parsed/$distance.csv" 
    while [ $i -le $2 ]
    do
        path="energy/telemetry-SF-$SF"
        mkdir -p "${path}/txt" "${path}/parsed" "${path}/csv" "${path}/loss"
        num=$(( $i ))
        #echo "NSta=$num" > "${path}/csv/$num.csv"

        ./waf --run "scratch/lora-telemetry.cc --simulationTime=$simulationTime --SF=$SF --payloadSize=$payloadSize --nDevices=1 --radius=$num" 2> "${path}/txt/$num.txt" > "${path}/loss/$num.txt"
        
        energyJoules=$(cat "${path}/txt/$num.txt" | grep -e "LoraRadioEnergyModel:Total energy consumption" | tail -1 | awk 'NF>1{print $NF}' | sed 's/J//g')
        echo "Energy in Joules: $energyJoules" 
        energyWatts=$(bc <<< "scale=10;($energyJoules/$simulationTime)*1000" | awk '{printf "%f", $0}') #Mili-watts
        echo "Energy in Milli-Watts: $energyWatts"
        nbPackets=$(cat "${path}/loss/$num.txt" | tail -1 | awk '{print $2;}')
        nbBytes=$(bc <<< "$nbPackets/$num*($payloadSize+9)")
        energyJoulesPerByte=$(bc <<< "scale=10;$energyJoules/$nbBytes" | awk '{printf "%f", $0}') #Joules/Byte
        echo "Energy in Joules/Byte: $energyJoulesPerByte"
        #echo $num, $energyJoules, $energyWatts, $energyJoulesPerByte >> "${path}/parsed/$distance.csv" 
        #cat "${path}/txt/$num.txt" | grep -e "LoraRadioEnergyModel:Total energy consumption" | tail -1 | awk 'NF>1{print $NF}' | sed 's/J//g' | awk -v s=$simulationTime '{print $1/s*1000}' > "${path}/parsed/$num-watts.txt"
        #python3 get_latencies.py "${path}/parsed/$num.txt" "${path}/csv/$num.csv"
        
        #python3 get_loss.py "${path}/loss/$num.txt" "${path}/loss/losses.csv" $num
        
        ((i=i+500)) #Step = 50
    done
done