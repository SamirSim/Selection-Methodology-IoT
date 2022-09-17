declare -a deviceModel="Soekris"
declare -a MCS=0
declare -a txPower=6 #dBm
declare -a simulationTime="5" #seconds
declare -a payloadSize="500" #bytes
declare -a i=3
declare -a step=1

mkdir -p "energy/ns3-model"
path="energy/ns3-model"
rm "${path}/$MCS-$txPower-$payloadSize.csv"

for dataRate in $(seq $1 $step $2)
do    
    ./waf --run "scratch/ns3-model.cc --simulationTime=$simulationTime --dataRate=$dataRate --payloadSize=$payloadSize --nWifi=$i --MCS=$MCS --txPower=$txPower" 2> "${path}/$dataRate-$MCS-$txPower-$payloadSize.txt"
    python3 get_energy_ns3_model.py "${path}/$dataRate-$MCS-$txPower-$payloadSize.txt" $simulationTime $dataRate >> "${path}/$MCS-$txPower-$payloadSize.csv"
    #python3 get_energy.py "${path}/$dataRate-$MCS-$txPower.tr" $simulationTime $payloadSize $deviceModel $MCS $txPower $dataRate
done