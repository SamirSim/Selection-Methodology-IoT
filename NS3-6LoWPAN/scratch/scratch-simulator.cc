#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-mac.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/core-module.h"
#include "ns3/energy-module.h"
#include "ns3/wifi-radio-energy-model-helper.h"
#include "ns3/applications-module.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/building.h"
#include "ns3/mobility-building-info.h"
#include "ns3/building-list.h"
#include "ns3/buildings-helper.h"
#include <iomanip>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("wifi-periodic");

 void
 RemainingEnergy (double oldValue, double remainingEnergy)
 {
   NS_LOG_UNCOND (Simulator::Now ().GetSeconds ()
                  << "s Current remaining energy = " << remainingEnergy << "J");
 }
 
 void
 TotalEnergy (double oldValue, double totalEnergy)
 {
   NS_LOG_UNCOND (Simulator::Now ().GetSeconds ()
                  << "s Total energy consumed by radio = " << totalEnergy << "J");
 }

int main (int argc, char *argv[]) {
  SeedManager::SetSeed (3);  // Changes seed from default of 1 to 3
  SeedManager::SetRun (7);  // Changes run number from default of 1 to 7
  /*/ Input parameters definition /*/
  // Seconds
  double simulationTime = 10; 
  // Number of stations
  int nWifi = 20; 
  // Number of gatewyas
  int nGW = 1; 
  // Modulation and Coding Scheme
  int MCS = 10;  // If 10 then IdealWifiManager
  // TxPower
  int txPower = 9; 
  // Traffic direction
  std::string trafficDirection = "upstream"; 
  // Payload size in bytes
  int payloadSize = 100; 
  // Packet period in seconds
  std::string period = "1"; 
  // Meters between AP and stations
  double distance = 50.0; 
  // Allow or not the packet agregation
  int agregation = 0; 
  // BW Channel Width in MHz
  int channelWidth = 80; 
  // Indicates whether Short Guard Interval is enabled or not
  int sgi = 0; 
  // Delay propagation model
  std::string propDelay = "ConstantSpeedPropagationDelayModel";
  // Radion environment
  std::string radioEnvironment = "urban";
  // Number of spatial streams 
  int spatialStreams = 3; 
  // Tx current draw in mA
  double txCurrent = 107;
  // Rx current draw in mA
  double rxCurrent = 40;
  // CCA_Busy current draw in mA
  double ccaBusyCurrent = 1;
  // Idle current draw in mA
  double idleCurrent = 1;

  bool latency = true;
  bool energyPower = true;
  bool energyRatio = false;
  bool crossFactor = false;
  bool batteryRV = false;
  bool successRate = true;

  bool hiddenStations = false;

  //Energy parameters
  double tx = 0.52;  // in W
  double rx = 0.16;  // in W
  double txFactor = 0.93; // in mJ
  double rxFactor = 0.93; // in mJ
  double voltage = 12; // in W
  double batteryCap = 5200; // Battery capacity in mAh

  CommandLine cmd (__FILE__);
  cmd.AddValue ("distance", "Distance in meters between the station and the access point", distance);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("MCS", "MCS", MCS);
  cmd.AddValue ("sgi", "SGI", sgi);
  cmd.AddValue ("agregation", "Allow agregation or not", agregation);
  cmd.AddValue ("txPower", "TxPower in dBm", txPower);
  cmd.AddValue ("payloadSize", "Payload size in Bytes", payloadSize);
  cmd.AddValue ("channelWidth", "Channel Width in MHz", channelWidth);
  cmd.AddValue ("propDelay", "Delay Propagation Model", propDelay);
  cmd.AddValue ("radioEnvironment", "Distance Propagation Model", radioEnvironment);
  cmd.AddValue ("spatialStreams", "Number of Spatial Streams", spatialStreams);
  cmd.AddValue ("batteryCap", "Battery Capacity in mAh", batteryCap);
  cmd.AddValue ("voltage", "Battery voltage in Volts", voltage);
  cmd.AddValue ("txCurrent", "Tx current draw in mA", txCurrent);
  cmd.AddValue ("rxCurrent", "Rx current draw in mA", rxCurrent);
  cmd.AddValue ("idleCurrent", "Idle current draw in mA", idleCurrent);
  cmd.AddValue ("ccaBusyCurrent", "CCA Busy voltage in Volts", ccaBusyCurrent);
  cmd.AddValue ("period", "Packet period in S", period);
  cmd.AddValue ("nWifi", "Number of stations", nWifi);
  cmd.AddValue ("nGW", "Number of gateways", nGW);
  cmd.AddValue ("trafficDirection", "Direction of traffic UL/DL", trafficDirection);
  cmd.AddValue ("latency", "Time a probing packets takes", latency);
  cmd.AddValue ("energyPower", "Energy consumption in Watts", energyPower);
  cmd.AddValue ("energyRatio", "Energy ratio in Joules/Byte", energyRatio);
  cmd.AddValue ("crossFactor", "Consider cross-factor or not", crossFactor);
  cmd.AddValue ("batteryRV", "Whether to use the RV model or not", batteryRV);
  cmd.AddValue ("successRate", "Percentage of successful packets", successRate);
  cmd.AddValue ("hiddenStations", "If there are hidden nodes or not", hiddenStations);
  cmd.Parse (argc,argv);

  if (latency) {
    LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
    LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);

    Time::SetResolution (Time::NS);
  }

  LogComponentEnableAll (LOG_PREFIX_FUNC);
  LogComponentEnableAll (LOG_PREFIX_NODE);
  LogComponentEnableAll (LOG_PREFIX_TIME);

  nWifi = (int) (nWifi / nGW);
  distance = distance / nGW;

  YansWifiChannelHelper channel;
  std::string propLoss;
  if (radioEnvironment == "urban") propLoss = "Cost231PropagationLossModel";
  else if (radioEnvironment == "suburban") propLoss = "LogDistancePropagationLossModel";
  else if (radioEnvironment == "rural") propLoss = "OkumuraHataPropagationLossModel";
  else if (radioEnvironment == "indoor") propLoss = "HybridBuildingsPropagationLossModel";

  channel.AddPropagationLoss ("ns3::"+propLoss);
  channel.SetPropagationDelay("ns3::"+propDelay);

  /*/ Nodes creation and placement /*/
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);
  NodeContainer wifiApNode;
  wifiApNode.Create (1);

  NodeContainer wifiProbingNode;

  if (latency) {
    wifiProbingNode.Create (1);
  }

   // Setting mobility model
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

  if (hiddenStations) {
    Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("999999"));
    // Set the maximum wireless range to 5 meters in order to reproduce a hidden nodes scenario, i.e. the distance between hidden stations is larger than 5 meters
    Config::SetDefault ("ns3::RangePropagationLossModel::MaxRange", DoubleValue (distance));

    channel.AddPropagationLoss ("ns3::RangePropagationLossModel"); //wireless range limited to (distance) meters! 

    positionAlloc->Add (Vector (distance, 0.0, 0.0));
    
    for (uint32_t i = 0; i < nWifi/2 ; i++) {
      positionAlloc->Add (Vector (0.0, 0.0, 0.0));
    }
    for (uint32_t i = 0; i < nWifi/2; i++) {
      positionAlloc->Add (Vector (2*distance, 0.0, 0.0));
    }
    // AP is between the two stations, each station being located at 5 meters from the AP.
    // The distance between the two stations is thus equal to 10 meters.
    // Since the wireless range is limited to 5 meters, the two stations are hidden from each other.
    
    mobility.SetPositionAllocator (positionAlloc);

    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

    mobility.Install (wifiApNode);
    mobility.Install (wifiStaNodes);
  }
  else {
    /*/ Positioning Nodes /*/
    /*
    for (uint32_t i = 0; i < nWifi; i++) {
        positionAlloc->Add (Vector (distance, 0.0, 0.0));
    }

    mobility.SetPositionAllocator (positionAlloc);
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (wifiStaNodes);

    Ptr<ListPositionAllocator> positionAp = CreateObject<ListPositionAllocator> ();
    positionAp->Add (Vector (0.0, 0.0, 0.0));
    mobility.SetPositionAllocator (positionAp);
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (wifiApNode);

    if (latency) {
      positionAp->Add (Vector (distance, 0.0, 0.0));
      mobility.SetPositionAllocator (positionAp);
      mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobility.Install (wifiProbingNode);
    }
    */
    mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator", "rho", DoubleValue (distance),
                                  "X", DoubleValue (0.0), "Y", DoubleValue (0.0), "Z", DoubleValue(1.0));
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiStaNodes);

    MobilityHelper mobilityAp;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    positionAlloc->Add (Vector (0, 0, 1));
    mobilityAp.SetPositionAllocator (positionAlloc);
    mobilityAp.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobilityAp.Install(wifiApNode);

    if (radioEnvironment == "indoor") {
      double x_min = 0.0;
      double x_max = 10.0;
      double y_min = 0.0;
      double y_max = 20.0;
      double z_min = 0.0;
      double z_max = 10.0;
      Ptr<Building> b = CreateObject <Building> ();
      b->SetBoundaries (Box (x_min, x_max, y_min, y_max, z_min, z_max));
      b->SetBuildingType (Building::Residential);
      b->SetExtWallsType (Building::ConcreteWithWindows);
      b->SetNFloors (3);
      b->SetNRoomsX (3);
      b->SetNRoomsY (2);

      BuildingsHelper::Install (wifiStaNodes);
      
      BuildingsHelper::Install (wifiApNode);
    }

    if (latency) {
      positionAlloc->Add (Vector (distance, 0, 1));
      mobility.SetPositionAllocator (positionAlloc);
      mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobility.Install (wifiProbingNode);
      if (radioEnvironment == "indoor") BuildingsHelper::Install (wifiProbingNode);
    }
  }
  /* Layers installation */
  YansWifiPhyHelper phy;
  phy.SetChannel (channel.Create ());

  phy.Set ("Antennas", UintegerValue (spatialStreams));
  phy.Set ("MaxSupportedTxSpatialStreams", UintegerValue (spatialStreams));
  phy.Set ("MaxSupportedRxSpatialStreams", UintegerValue (spatialStreams));

  WifiMacHelper mac;
  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211ac);

  if (MCS == 10) {
    wifi.SetRemoteStationManager("ns3::IdealWifiManager");
  }
  else {
    std::ostringstream oss;
    oss << "VhtMcs" << MCS;
    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                  "DataMode", StringValue (oss.str ()),
                                  "ControlMode", StringValue (oss.str ()));
  }

  Ssid ssid = Ssid ("ns3-80211ac");

  // Installing phy & mac layers on the stations
  mac.SetType ("ns3::StaWifiMac",
              "Ssid", SsidValue (ssid));
  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  NetDeviceContainer probingDevice;
  if (latency) {
    probingDevice = wifi.Install(phy, mac, wifiProbingNode);
  }

  // Installing phy & mac layers on the AP
  mac.SetType ("ns3::ApWifiMac",   
              "EnableBeaconJitter", BooleanValue (false),
              "Ssid", SsidValue (ssid));
  NetDeviceContainer apDevice;
  apDevice = wifi.Install (phy, mac, wifiApNode);


  /*/ Low-level parameters configuration /*/
  // Set channel width
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", UintegerValue (channelWidth));

  // Set guard interval
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HtConfiguration/ShortGuardIntervalSupported", BooleanValue (sgi));

  /* Internet stack*/
  InternetStackHelper stack;
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  if (latency) {
    stack.Install (wifiProbingNode);
  }

  /*/ IP addresses configuration /*/
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.0.0");
  Ipv4InterfaceContainer ApInterface;
  ApInterface = address.Assign (apDevice);
  Ipv4InterfaceContainer StaInterface;
  StaInterface = address.Assign (staDevices);

  Ipv4InterfaceContainer wifiProbingInterface;
  if (latency) {
    wifiProbingInterface = address.Assign(probingDevice);
  }

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /*/ Setting probing application /*/
  if (latency) {
    // UDP Echo Server application to be installed in the AP
    int echoPort = 11;
    UdpEchoServerHelper echoServer(echoPort); // Port # 9
    uint32_t payloadSizeEcho = 1023; //Packet size for Echo UDP App

    if (trafficDirection == "upstream") {
      ApplicationContainer serverApps = echoServer.Install(wifiApNode);
      serverApps.Start(Seconds(0.0));
      serverApps.Stop(Seconds(simulationTime+1));
      
      //wifiApInterface.GetAddress(0).Print(std::cout);
      // UDP Echo Client application to be installed in the probing station
      UdpEchoClientHelper echoClient1(ApInterface.GetAddress(0), echoPort);
      
      echoClient1.SetAttribute("MaxPackets", UintegerValue(10000));
      echoClient1.SetAttribute("Interval", TimeValue(Seconds(0.05)));
      echoClient1.SetAttribute("PacketSize", UintegerValue(payloadSizeEcho));

      ApplicationContainer clientApp = echoClient1.Install(wifiProbingNode);
      //commInterfaces.GetAddress(0).Print(std::cout);
      clientApp.Start(Seconds(1.0));
      clientApp.Stop(Seconds(simulationTime+1));
    }
    else {
      ApplicationContainer serverApps = echoServer.Install(wifiProbingNode);
      serverApps.Start(Seconds(0.0));
      serverApps.Stop(Seconds(simulationTime+1));

      // UDP Echo Client application to be installed in the probing station
      UdpEchoClientHelper echoClient1(wifiProbingInterface.GetAddress(0), echoPort);
      
      echoClient1.SetAttribute("MaxPackets", UintegerValue(10000));
      echoClient1.SetAttribute("Interval", TimeValue(Seconds(2)));
      echoClient1.SetAttribute("PacketSize", UintegerValue(payloadSizeEcho));

      ApplicationContainer clientApps = echoClient1.Install(wifiApNode);
      //commInterfaces.GetAddress(0).Print(std::cout);
      clientApps.Start(Seconds(1.0));
      clientApps.Stop(Seconds(simulationTime+1));
    }
  }

  if (agregation == 0) {
    // Disable A-MPDU & A-MSDU in AP
    Ptr<NetDevice> dev = wifiApNode.Get(0)-> GetDevice(0);
    Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice> (dev);
    wifi_dev->GetMac ()->SetAttribute ("BE_MaxAmpduSize", UintegerValue (0));
    wifi_dev->GetMac ()->SetAttribute ("BE_MaxAmsduSize", UintegerValue (0));
  }
  // Set txPower in the stations
  for (uint32_t index = 0; index < nWifi; ++index) {
    Ptr<WifiPhy> phy_tx = dynamic_cast<WifiNetDevice*>(GetPointer((staDevices.Get(index))))->GetPhy();
    phy_tx->SetTxPowerEnd(txPower);
    phy_tx->SetTxPowerStart(txPower);
  }

  // Set txPower in the AP
  Ptr<WifiPhy> phy_tx = dynamic_cast<WifiNetDevice*>(GetPointer((apDevice.Get(0))))->GetPhy();
  phy_tx->SetTxPowerEnd(txPower);
  phy_tx->SetTxPowerStart(txPower);

  if (latency) {
    Ptr<WifiPhy> phy_tx = dynamic_cast<WifiNetDevice*>(GetPointer((probingDevice.Get(0))))->GetPhy();
    phy_tx->SetTxPowerEnd(txPower);
    phy_tx->SetTxPowerStart(txPower);
  }

  /*/ Setting traffic applications /*/
  ApplicationContainer sourceApplications, sinkApplications;
  uint32_t portNumber = 9;
  double min = 0.5;
  double max = 1.0;
  double periodSeconds = std::stof(period);

  if (trafficDirection == "upstream") {
    auto ipv4 = wifiApNode.Get (0)->GetObject<Ipv4> ();
    const auto address = ipv4->GetAddress (1, 0).GetLocal ();
    InetSocketAddress sinkSocket (address, portNumber);
    PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", sinkSocket);
    sinkApplications.Add (packetSinkHelper.Install (wifiApNode.Get (0)));
    
    for (uint32_t index = 0; index < nWifi; ++index) {
      if (agregation == 0) {
        // Disable A-MPDU & A-MSDU in each station
        Ptr<NetDevice> dev = wifiStaNodes.Get (index)->GetDevice (0);
        Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice> (dev);
        wifi_dev->GetMac ()->SetAttribute ("BE_MaxAmpduSize", 
                                          UintegerValue (0));
        wifi_dev->GetMac ()->SetAttribute ("BE_MaxAmsduSize", 
                                          UintegerValue (0));
      }
      
      // UDP Client application to be installed in the stations
      UdpClientHelper echoClient(address, portNumber);
    
      echoClient.SetAttribute("MaxPackets", UintegerValue(100000));
      echoClient.SetAttribute("Interval", TimeValue(Seconds(periodSeconds)));
      echoClient.SetAttribute("PacketSize", UintegerValue(payloadSize));

      // Desynchronize the sending applications
      Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
      x->SetAttribute ("Min", DoubleValue (min));
      x->SetAttribute ("Max", DoubleValue (max));

      double value = 1 + x->GetValue ();

      sourceApplications = echoClient.Install (wifiStaNodes.Get(index));
      sourceApplications.Start(Seconds(value));
      sourceApplications.Stop(Seconds(simulationTime+value));
    }
  }
  else {
    for (uint32_t index = 0; index < nWifi; ++index) {
      if (agregation == 0) {
        // Disable A-MPDU & A-MSDU in each station
        Ptr<NetDevice> dev = wifiStaNodes.Get (index)->GetDevice (0);
        Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice> (dev);
        wifi_dev->GetMac ()->SetAttribute ("BE_MaxAmpduSize", UintegerValue (0));
        wifi_dev->GetMac ()->SetAttribute ("BE_MaxAmsduSize", UintegerValue (0));
      }

      auto ipv4 = wifiStaNodes.Get (index)->GetObject<Ipv4> ();
      const auto address = ipv4->GetAddress (1, 0).GetLocal ();
      InetSocketAddress sinkSocket (address, portNumber);
      
      // UDP Client application to be installed in the stations
      UdpClientHelper echoClient2(address, portNumber);
    
      echoClient2.SetAttribute("MaxPackets", UintegerValue(1000000));
      echoClient2.SetAttribute("Interval", TimeValue(Seconds(std::stof(period))));
      echoClient2.SetAttribute("PacketSize", UintegerValue(payloadSize));
      
      Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
      x->SetAttribute ("Min", DoubleValue (min));
      x->SetAttribute ("Max", DoubleValue (max));

      double value = 1 + x->GetValue ();

      ApplicationContainer sourceApplications1 = echoClient2.Install (wifiApNode.Get(0));
      sourceApplications1.Start(Seconds(value));
      sourceApplications1.Stop(Seconds(simulationTime+value));
      
      PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", sinkSocket);
      sinkApplications.Add(packetSinkHelper.Install (wifiStaNodes.Get (index)));    
    }
  }
  /*/ Starting applications /*/
  sinkApplications.Start (Seconds (0.0));
  sinkApplications.Stop (Seconds (simulationTime + 2));

  /*/ Installing energy models /*/
  DeviceEnergyModelContainer deviceModels;

  double capacityJoules = (batteryCap / 1000.0) * voltage * 3600;

  WifiRadioEnergyModelHelper radioEnergyHelper;

  radioEnergyHelper.Set ("IdleCurrentA", DoubleValue (idleCurrent/1000));
  radioEnergyHelper.Set ("TxCurrentA", DoubleValue (txCurrent/1000));
  radioEnergyHelper.Set ("CcaBusyCurrentA", DoubleValue (ccaBusyCurrent/1000));
  radioEnergyHelper.Set ("RxCurrentA", DoubleValue (rxCurrent/1000));

  if (!batteryRV) { // If the battery model is linear
    BasicEnergySourceHelper basicSourceHelper;
    basicSourceHelper.Set ("BasicEnergySupplyVoltageV", 
                          DoubleValue (voltage));
    basicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", 
                          DoubleValue (capacityJoules));
    EnergySourceContainer sources = basicSourceHelper.Install(wifiStaNodes);

    deviceModels = radioEnergyHelper.Install (staDevices, sources);
  }
  else { // If the battery model is RV
    RvBatteryModelHelper rvModelHelper;
    rvModelHelper.Set ("RvBatteryModelOpenCircuitVoltage", 
                      DoubleValue(2*voltage));
    rvModelHelper.Set ("RvBatteryModelCutoffVoltage", 
                      DoubleValue(0)); 
    rvModelHelper.Set ("RvBatteryModelAlphaValue", 
                      DoubleValue(capacityJoules / voltage));
    EnergySourceContainer sources = rvModelHelper.Install(wifiStaNodes);
    WifiRadioEnergyModelHelper radioEnergyHelper;
    
    deviceModels = radioEnergyHelper.Install (staDevices, sources);
  }    
  //std::string s = "telemetry-23Bytes/"+std::to_string(nWifi)+"-"+period+"-"+std::to_string(MCS)+"-"+std::to_string(payloadSize);
  
  /*/ Traces files generation /*/
  AsciiTraceHelper ascii;
  phy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
  std::string s = "trace";
  phy.EnableAsciiAll (ascii.CreateFileStream(s+".tr"));
  phy.EnablePcap (s+".pcap", apDevice.Get(0), false, true);

  /*/ Starting simulation /*/
  Simulator::Stop (Seconds (simulationTime + 2));
  Simulator::Run ();

  std::cout << std::fixed;
  std::cout << std::setprecision(2);
  
  /*/ Gatherting KPIs /*/
  if (energyRatio || energyPower) {
    /*/ Calculating Energy KPIs /*/
    double energy = 0, battery_lifetime = 0;
    DeviceEnergyModelContainer::Iterator iter ;
    for (iter = deviceModels.Begin (); iter != deviceModels.End (); iter ++) {
      double energyConsumed = (*iter)->GetTotalEnergyConsumption ();
      NS_LOG_UNCOND ("End of simulation (" << Simulator::Now ().GetSeconds ()
                    << "s) Total energy consumed by radio (Station) = " 
                    << energyConsumed << "J");
      std::cout << "Total energy consumed by radio (Station): " 
                << energyConsumed << std::endl;
      battery_lifetime = ((capacityJoules / energyConsumed) * simulationTime);
      battery_lifetime = battery_lifetime / 86400; // Days
      std::cout << "Battery lifetime: " << battery_lifetime << std::endl;
      energy = energyConsumed;
      break; // Energy in only one station
    }
  }

  double totalPacketsThrough = 0, throughput = 0;
  if (trafficDirection == "upstream") {
    for (uint32_t index = 0; index < sinkApplications.GetN (); ++index) {
      totalPacketsThrough = DynamicCast<PacketSink> (sinkApplications.Get (index)) ->GetTotalRx ();
      throughput += ((totalPacketsThrough * 8) / ((simulationTime) * 1024)); //Mbit/s
      std::cout << "Throughput: " << throughput << std::endl;
    }
  }
  else {
    for (uint32_t index = 0; index < sinkApplications.GetN (); ++index) {
      totalPacketsThrough += DynamicCast<PacketSink> (sinkApplications.Get (index))->GetTotalRx ();
    }
  }
  double totalSent;
  if (trafficDirection == "upstream") {
    for (uint32_t index = 0; index < sourceApplications.GetN (); ++index) {
      totalSent += DynamicCast<UdpClient> (sourceApplications.Get (index)) ->GetTotalTx ();
    }
  }

  totalSent *= nWifi;

  if (energyRatio) {
    double receivedByteStation = totalPacketsThrough / nWifi; // Number of successfuly received bytes from one station
    std::cout << "Number of received bytes from one station: " << receivedByteStation << std::endl;
  }

  /*
  if (crossFactor) {
    std::cout << "Number of generated packets by one station: " << totalSent / nWifi << std::endl; // Number of generated packets by station
  }
  */

  if (successRate) {
    double wholeSuccessRate =  (totalPacketsThrough / totalSent) * 100;
    std::cout << "Success rate: " <<  wholeSuccessRate << std::endl; // Success rate percentage
  }
  
  Simulator::Destroy ();
  return 0;
}