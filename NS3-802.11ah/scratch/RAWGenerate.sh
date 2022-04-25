#!/bin/bash

if [ $# -ne 3 ]
then
echo "parameters missing"
exit 1
fi

NumSta=$1
#antho=$2
NRawGroups=$2
beaconinterval=$3
NumSlot=2
pageSliceCount=1
pageSliceLen=0

RAWConfigPath="./OptimalRawGroup/RawConfig-testing.txt"
#-$pagePeriod-$pageSliceLength-$pageSliceCount

./waf --run "RAW-generate --NRawSta=$NumSta --NGroup=$NRawGroups --NumSlot=$NumSlot --RAWConfigPath=$RAWConfigPath --beaconinterval=$beaconinterval --pageSliceCount=$pageSliceCount --pageSliceLen=$pageSliceLen"

#./waf --run RAW-generate --command-template="gdb --args %s <args> --NRawSta=$NumSta --NGroup=$NRawGroups --NumSlot=$NumSlot --RAWConfigPath=$RAWConfigPath --beaconinterval=$beaconinterval --pageSliceCount=$pageSliceCount --pageSliceLen=$pageSliceLen"