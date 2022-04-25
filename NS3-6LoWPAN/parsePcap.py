import argparse
import os
import sys
import pyshark

files = ["trace.pcap"]

for file in files:
    count = 0

    cap = pyshark.FileCapture(file, display_filter='ip.src==10.0.0.2 and ip.dst==10.0.0.1')
    print(len(cap))

    for packet in cap:
        if 'IP' in packet:
            src = packet['IP'].src
            if src == '10.0.0.2':
                flags = packet['WLAN'].Flags
                if flags == "0x00000009": #Frame being retransmitted
                    count = count + 1
                    #print(count)

    print(file, count)
