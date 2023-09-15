# Selection-Methodology-IoT
This code corresponds to the HINTS methodology described in the following paper (please cite this in case you use the code):

```
@article{si2023hints,
  title={HINTS: A methodology for IoT network technology and configuration decision},
  author={Si-Mohammed, Samir and Begin, Thomas and Lassous, Isabelle Gu{\'e}rin and Vicat-Blanc, Pascale},
  journal={Internet of Things},
  volume={22},
  pages={100678},
  year={2023},
  publisher={Elsevier}
}
```

The first thing to do is to build the ns-3 environments. To do so, go inside the `NS3-6LOWPAN/`, `NS3-5G/` and `NS3-802.11ah/` folders, and run the following commands:
```
./waf configure --disable-werror
./waf build
```
## Automatic Simulation and Comparison
The main script for running the simulation and the comparison is `methodology.py`. All the parameters (both application- and network-related) can be set in that file.
## Specific Simulation
It is also possible to run specific scripts without the automatic selection process. To do so:
### Wi-Fi, 6LoWPAN and LoRa
Go to `NS3-6LOWPAN/scratch/` and update one of the `*-periodic.cc` files. You can then run the simulation using `./waf --run scratch/{filename}.cc`  
### 5G mmWave
Go to `NS3-5G/scratch/` and update the `5g-periodic.cc` file. You can then run the simulation using `./waf --run scratch/{filename}.cc`
### 802.11ah
Go to `NS3-802.11ah/scratch/rca` and update the `s1g-rca.cc` file. You can then run the simulation using `./waf --run rca`. Note that some of the configuration parameters of the simulation can be found in `Configuration.h`.

