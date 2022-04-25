declare -a deviceModel="Soekris"
declare -a MCS=0
declare -a txPower=6 #dBm
declare -a simulationTime="5" #seconds
declare -a payloadSize="500" #bytes
declare -a i=3
declare -a step=1

mkdir -p "energy/serrano"
path="energy/serrano"
rm "${path}/$MCS-$txPower-$payloadSize.txt"
rm "${path}/$MCS-$txPower-$payloadSize.csv"

for dataRate in $(seq $1 $step $2)
do    
    ./waf --run "scratch/serrano-model.cc --simulationTime=$simulationTime --dataRate=$dataRate --payloadSize=$payloadSize --nWifi=$i --MCS=$MCS --txPower=$txPower" 
    python3 get_energy_serrano_model.py "${path}/$dataRate-$MCS-$txPower-$payloadSize.tr" $simulationTime $payloadSize $deviceModel $MCS $txPower $dataRate >> "${path}/$MCS-$txPower-$payloadSize.txt"
    #python3 get_energy_serrano_model.py "${path}/$dataRate-$MCS-$txPower-$payloadSize.tr" $simulationTime $payloadSize $deviceModel $MCS $txPower $dataRate
done

python3 plot_energy.py "${path}/$MCS-$txPower-$payloadSize.txt" "${path}/$MCS-$txPower-$payloadSize.csv"