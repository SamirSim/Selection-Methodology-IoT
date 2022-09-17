declare -a deviceModel="Soekris"
declare -a MCS=1
declare -a txPower=9 #dBm
declare -a simulationTime="5" #seconds
declare -a payloadSize="500" #bytes
declare -a i=3
declare -a step=1
declare -a trafficDirection="downstream"
declare -a period=1

mkdir -p "energy/final"
path="energy/final"
rm "${path}/$MCS-$txPower-$payloadSize-$trafficDirection.csv"


./waf --run "scratch/script-periodic-802.11-ac.cc --trafficDirection=$trafficDirection --simulationTime=$simulationTime --period=$period --payloadSize=$payloadSize --nWifi=$i --MCS=$MCS --txPower=$txPower" 2> "${path}/$period-$MCS-$txPower-$payloadSize-$trafficDirection.txt"
python3 get_energy_ns3_model.py "${path}/$period-$MCS-$txPower-$payloadSize-$trafficDirection.txt" $simulationTime $period > "${path}/cross-factor-buffer.txt"
python3 get_cross_factor.py "${path}/$period-$MCS-$txPower-$payloadSize-$trafficDirection.tr" "${path}/cross-factor-buffer.txt" $simulationTime $payloadSize $deviceModel "${path}/$MCS-$txPower-$payloadSize-$trafficDirection.csv"
#python3 get_energy.py "${path}/$period-$MCS-$txPower.tr" $simulationTime $payloadSize $deviceModel $MCS $txPower $period
