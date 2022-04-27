/*
 * This script simulates a complex scenario with multiple gateways and end
 * devices. The metric of interest for this script is the throughput of the
 * network.
 */

#include "ns3/end-device-lora-phy.h"
#include "ns3/gateway-lora-phy.h"
#include "ns3/class-a-end-device-lorawan-mac.h"
#include "ns3/gateway-lorawan-mac.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/okumura-hata-propagation-loss-model.h"
#include "ns3/cost231-propagation-loss-model.h"
#include "ns3/lora-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/node-container.h"
#include "ns3/mobility-module.h"
#include "ns3/position-allocator.h"
#include "ns3/periodic-sender-helper.h"
#include "ns3/command-line.h"
#include "ns3/basic-energy-source-helper.h"
#include "ns3/lora-radio-energy-model-helper.h"
#include "ns3/file-helper.h"
#include "ns3/names.h"
#include "ns3/core-module.h"
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <fstream>

using namespace ns3;
using namespace lorawan;

void PrintPositions (NodeContainer loraNodes)
{
  
    Ptr<RandomWalk2dMobilityModel> mob = loraNodes.Get(1)->GetObject<RandomWalk2dMobilityModel>();
    Vector pos = mob->GetPosition ();
    std::cout << "POS: x=" << pos.x << ", y=" << pos.y << ", z=" << pos.z << "," << Simulator::Now ().GetSeconds ()<< std::endl;
    //mob->SetVelocity (Vector(1,0,0));
    
    Simulator::Schedule(Seconds(1), &PrintPositions, loraNodes);
}

NS_LOG_COMPONENT_DEFINE ("lora-periodic");

int
main (int argc, char *argv[]) {
  /*/ Defining input parameters /*/
  double simulationTime = 600;
  int nSta = 50;
  int nGateways = 1;
  double SF=0; // If SF=0, then the SF is set automatically
  uint8_t codingRate=1;
  int crc = 0;
  double payloadSize = 50;
  int period = 60; //One packet per hour
  double distance = 7000;
  std::string trafficType = "Unconfirmed";

  bool energyRatio = true;
  bool successRate = false;
  bool batteryLife = false;
  bool mobileNodes = true;

  int channelWidth = 80; // BW Channel Width in MHz
  std::string propDelay = "ConstantSpeedPropagationDelayModel";
  std::string propLoss = "LogDistancePropagationLossModel";

  CommandLine cmd;
  cmd.AddValue ("nSta", "Number of end devices to include in the simulation", nSta);
  cmd.AddValue ("nGateways", "Number of gateways", nGateways);
  cmd.AddValue ("distance", "The distance of the area to simulate", distance);
  cmd.AddValue ("SF", "Fixed spreading factor", SF);
  cmd.AddValue ("codingRate", "Coding Rate", codingRate);
  cmd.AddValue ("mobileNodes", "Mobile nodes or not", mobileNodes);
  cmd.AddValue ("crc", "Cyclic Redundancy Check", crc);
  cmd.AddValue ("trafficType", "Confirmed or unconfirmed traffic", trafficType);
  cmd.AddValue ("simulationTime", "The time for which to simulate", simulationTime);
  cmd.AddValue ("payloadSize", "The size of the payload", payloadSize);
  cmd.AddValue ("period", "The period in seconds to be used by periodically transmitting applications", period);
  cmd.AddValue ("channelWidth", "Channel Width in MHz", channelWidth);
  //cmd.AddValue ("topologyFile", "Topology file name", topologyFile);
  cmd.AddValue ("propDelay", "Delay Propagation Model", propDelay);
  cmd.AddValue ("propLoss", "Distance Propagation Model", propLoss);
  cmd.AddValue ("energyRatio", "Energy ratio in Joules/Byte", energyRatio);
  cmd.AddValue ("successRate", "Percentage of successful packets", successRate);
  cmd.AddValue("batteryLife", "Percentage of successful packets", batteryLife);
  cmd.Parse (argc, argv);

  // In case we have several gateways, divide the number of stations and the distance to the GW
  distance = distance / nGateways;
  nSta = (int) (nSta / nGateways);

  if (energyRatio) {
    LogComponentEnable ("lora-periodic", LOG_LEVEL_ALL);
    LogComponentEnable ("LoraRadioEnergyModel", LOG_LEVEL_ALL);
    LogComponentEnable("EndDeviceLorawanMac", LOG_LEVEL_ALL);
    LogComponentEnable("GatewayLorawanMac", LOG_LEVEL_ALL);
    LogComponentEnable("LoraPhy", LOG_LEVEL_ALL);
  }

  LogComponentEnableAll (LOG_PREFIX_FUNC);
  LogComponentEnableAll (LOG_PREFIX_NODE);
  LogComponentEnableAll (LOG_PREFIX_TIME);

  // Mobility
  MobilityHelper mobility;
  /*
  Ptr<ListPositionAllocator> allocator2 = CreateObject<ListPositionAllocator> ();
  for (uint32_t i = 0; i < nSta; i++) {
    allocator2->Add (Vector (distance,0,1));
  }
  mobility.SetPositionAllocator (allocator2);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  */

  if (mobileNodes) {
    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                    "MinX", DoubleValue (0.0),
                                    "MinY", DoubleValue (0.0),
                                    "DeltaX", DoubleValue (10.0),
                                    "DeltaY", DoubleValue (10.0),
                                    "GridWidth", UintegerValue (3),
                                    "LayoutType", StringValue ("RowFirst"));

      //mobility.SetPositionAllocator (position);
      mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                                "Mode", StringValue ("Time"),
                                "Time", StringValue ("2s"),
                                "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=20.0]"),
                                "Bounds", RectangleValue (Rectangle (-distance/2, distance/2, -distance/2, distance/2)));
  } 
  else {
    mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator", "rho", DoubleValue (distance),
                                 "X", DoubleValue (0.0), "Y", DoubleValue (0.0), "Z", DoubleValue(1.0));
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  }

  Ptr<LoraChannel> channel;
  Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel> ();

  // Create the lora channel object
  if (propLoss == "LogDistancePropagationLossModel") {
    Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel> ();
    loss->SetPathLossExponent (3.76);
    loss->SetReference (1, 7.7);
    channel = CreateObject<LoraChannel> (loss, delay);
  }
  else if (propLoss == "OkumuraHataPropagationLossModel") {
    Ptr<OkumuraHataPropagationLossModel> loss = CreateObject<OkumuraHataPropagationLossModel> ();
    //loss->SetPathLossExponent (3.76);
    //loss->SetReference (1, 7.7);
    channel = CreateObject<LoraChannel> (loss, delay);
  }

  // Installing phy & mac layers on the overloading stations
  LoraPhyHelper phyHelper = LoraPhyHelper ();
  phyHelper.SetChannel (channel);

  // Create the LorawanMacHelper
  LorawanMacHelper macHelper = LorawanMacHelper ();

  // Create the LoraHelper
  LoraHelper helper = LoraHelper ();
  helper.EnablePacketTracking (); // Output filename

  // Create a set of nodes
  NodeContainer endDevices;
  endDevices.Create (nSta);

  // Assign a mobility model to each node
  mobility.Install (endDevices);

  PrintPositions (endDevices);

  // Create the LoraNetDevices of the end devices
  phyHelper.SetDeviceType (LoraPhyHelper::ED);
  macHelper.SetDeviceType (LorawanMacHelper::ED_A);
  NetDeviceContainer endDevicesNetDevices = helper.Install (phyHelper, macHelper, endDevices);

  // Create the gateway nodes (allocate them uniformely on the disc)
  NodeContainer gateways;
  gateways.Create (1);

  Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator> ();
  // Make it so that nodes are at a certain height > 0
  allocator->Add (Vector (0.0, 0.0, 50.0));
  mobility.SetPositionAllocator (allocator);
  mobility.Install (gateways);

  /*
  CsvReader csv (topologyFile);

    while (csv.FetchNextRow ()) {
        if (csv.IsBlankRow ()) {
          continue;
        }    
        // Read the trace and get the arrival information
      double x, y, z;
      bool ok1, ok2, ok3;
      ok1 = csv.GetValue (0, x);
      ok2= csv.GetValue (1, y);
      ok3 = csv.GetValue (2, z);
      
      for (uint32_t i = 0; i < nSta; i++) {
        csv.FetchNextRow ();
        ok1 = csv.GetValue (0, x);
        ok2= csv.GetValue (1, y);
        ok3 = csv.GetValue (2, z);
        positionAlloc->Add (Vector (x, y, z));
        //std::cout << x << " and " << y << " and " << z << std::endl;
      }
      mobility.SetPositionAllocator (positionAlloc);
      mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobility.Install(endDevices);

      for (uint32_t i = 0; i < nGateways; i++) {
        csv.FetchNextRow ();
        ok1 = csv.GetValue (0, x);
        ok2= csv.GetValue (1, y);
        ok3 = csv.GetValue (2, z);
        positionGW->Add (Vector (x, y, z));
        //std::cout << x << " AP and " << y << " and " << z << std::endl;
      }
    
      mobility.SetPositionAllocator (positionGW);
      mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobility.Install(gateways);
      break;
    }
  */

  // Create a netdevice for each gateway
  phyHelper.SetDeviceType (LoraPhyHelper::GW);
  macHelper.SetDeviceType (LorawanMacHelper::GW);
  helper.Install (phyHelper, macHelper, gateways);

  macHelper.SetSpreadingFactorsUp (endDevices, gateways, channel, SF, codingRate, crc, trafficType);

  NS_LOG_DEBUG ("Completed configuration");

  /*/ Setting traffic applications /*/
  Time appStopTime = Seconds (simulationTime);
  PeriodicSenderHelper appHelper = PeriodicSenderHelper ();
  appHelper.SetPeriod (Seconds (period));
  appHelper.SetPacketSize (payloadSize);
  ApplicationContainer appContainer = appHelper.Install (endDevices);

  /*/ Installing energy models /*/
  if (energyRatio) {    
    BasicEnergySourceHelper basicSourceHelper;
    LoraRadioEnergyModelHelper radioEnergyHelper;

    // configure energy source
    basicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (10000)); // Energy in Joules
    basicSourceHelper.Set ("BasicEnergySupplyVoltageV", DoubleValue (3.3));

    radioEnergyHelper.Set ("StandbyCurrentA", DoubleValue (0.0014)); // Idle mode
    radioEnergyHelper.Set ("TxCurrentA", DoubleValue (0.028));
    radioEnergyHelper.Set ("SleepCurrentA", DoubleValue (0.0000015));
    radioEnergyHelper.Set ("RxCurrentA", DoubleValue (0.0112));

    //radioEnergyHelper.SetTxCurrentModel ("ns3::ConstantLoraTxCurrentModel",
      //                                  "TxCurrent", DoubleValue (0.028));

    radioEnergyHelper.SetTxCurrentModel ("ns3::LinearLoraTxCurrentModel");

    // install source on EDs' nodes
    EnergySourceContainer sources = basicSourceHelper.Install (endDevices);
    Names::Add ("/Names/EnergySource", sources.Get (0));

    // install device model
    DeviceEnergyModelContainer deviceModels = radioEnergyHelper.Install (endDevicesNetDevices, sources);
    
    if (batteryLife) {
      FileHelper fileHelper;
      fileHelper.ConfigureFile ("battery-level", FileAggregator::SPACE_SEPARATED);
      fileHelper.WriteProbe ("ns3::DoubleProbe", "/Names/EnergySource/RemainingEnergy", "Output");
    } 
  }
  
  Simulator::Stop (appStopTime);

  NS_LOG_INFO ("Running simulation...");

  Simulator::Run ();

  Simulator::Destroy ();

  LoraPacketTracker &tracker = helper.GetPacketTracker ();

  std::cout << std::fixed;
  std::cout << std::setprecision(2);

  /*/ Gatherting KPIs /*/
  std::cout << "Success rate: " << std::to_string(tracker.CountMacPacketsGlobally (Seconds (0), appStopTime)) << std::endl;
  std::cout << "Throughput: " << std::to_string((tracker.CountMacPacketsReceived (Seconds (0), appStopTime)*payloadSize*8)/simulationTime/1024) << std::endl; // kbps

  return 0;
}