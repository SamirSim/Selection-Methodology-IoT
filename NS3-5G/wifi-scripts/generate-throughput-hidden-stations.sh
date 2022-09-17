declare -a trafficDirections=("upstream" "downstream" )

for trafficDirection in ${trafficDirections[@]}; do
    path="wifi/overload-hidden/throughput/$trafficDirection/ac/graphs"
    mkdir "${path}/txt" "${path}/csv"
    
    for i in "$@"
    do
        ./waf --run "scratch/scratch-simulator.cc --nWifi=$i --trafficDirection=$trafficDirection" 1>> "${path}/txt/$i.txt"
        cat "${path}/txt/$i.txt" | grep -v '[a-z]' >> "${path}/txt/all.txt"
    done
    python3 get_throughputs.py "${path}/txt/all.txt" "${path}/csv/$i.csv"
done