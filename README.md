# Selection-Methodology-IoT
The first thing to do is to build the ns-3 environments. To do so, go inside the `NS3-6LOWPAN`, `NS3-5G` and `NS3-802.11ah` folder, and run the following commands:
```
./waf configure --disable-werror
./waf build
```
## Automatic Simulation and Comparison
The main script for running the simulation and the comparison is `methodology.py`. All the parameters (both application- and network-related) can be set in that file.
## Specific Simulation
It is also possible to run specific scripts without the automatic selection process. To do so:
### Wi-Fi, 6LoWPAN and LoRa
Go to `NS3-6LOWPAN/scratch` and update the `*-periodic.cc` files. You can then run the simulation using `./waf --run scratch/{filename}.cc`  
