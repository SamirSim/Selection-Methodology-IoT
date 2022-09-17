/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
*   Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
*   Copyright (c) 2015, NYU WIRELESS, Tandon School of Engineering, New York University
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License version 2 as
*   published by the Free Software Foundation;
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*   Author: Marco Miozzo <marco.miozzo@cttc.es>
*           Nicola Baldo  <nbaldo@cttc.es>
*
*   Modified by: Marco Mezzavilla < mezzavilla@nyu.edu>
*                         Sourjya Dutta <sdutta@nyu.edu>
*                         Russell Ford <russell.ford@nyu.edu>
*                         Menglei Zhang <menglei@nyu.edu>
*/


#include "ns3/mmwave-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store-module.h"
#include "ns3/command-line.h"
#include "ns3/mmwave-point-to-point-epc-helper.h"
#include "ns3/basic-energy-source-helper.h"
#include "ns3/mmwave-energy-helper.h"
#include "ns3/mmwave-radio-energy-model-enb-helper.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/building.h"
#include "ns3/mobility-building-info.h"
#include "ns3/building-list.h"
#include "ns3/buildings-helper.h"

using namespace ns3;
using namespace mmwave;

/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,
 * attaches one UE per eNodeB starts a flow for each UE to  and from a remote host.
 * It also  starts yet another flow between each UE pair.
 */
NS_LOG_COMPONENT_DEFINE ("5GPeriodic");
int
main (int argc, char *argv[])
{
  uint16_t numEnb = 1;
  uint16_t numUe = 10; // Including Probing node
  double simulationTime = 10.0;
  double interPacketInterval = 1;
  std::string trafficDirection = "upstream";
  double payloadSize = 100;
  double distance = 50.0; // eNB-UE distance in meters
  std::string radioEnvironment = "indoor";

  bool harqEnabled = false;
  bool rlcAmEnabled = false;
  double voltage = 12; // in W
  double batteryCap = 5200; // Battery capacity in mAh
  std::string numerology = "2";
  
  // Tx current draw in mA
  double txCurrent = 350;
  // Rx current draw in mA
  double rxCurrent = 350;
  // Idle current draw in mA
  double sleepCurrent = 45;

  bool latency = true;

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue ("numEnb", "Number of eNBs", numEnb);
  cmd.AddValue ("numUe", "Number of UEs per eNB", numUe);
  cmd.AddValue ("simulationTime", "Total duration of the simulation [s])", simulationTime);
  cmd.AddValue ("interPacketInterval", "Inter-packet interval [us])", interPacketInterval);
  cmd.AddValue ("trafficDirection", "Traffic direction (upstream or downstream)", trafficDirection);
  cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
  cmd.AddValue ("distance", "Max distance between UE and ENB in meters", distance);
  cmd.AddValue ("radioEnvironment", "Distance Propagation Model", radioEnvironment);
  cmd.AddValue ("harq", "Enable Hybrid ARQ", harqEnabled);
  cmd.AddValue ("rlcAm", "Enable RLC-AM", rlcAmEnabled);
  cmd.AddValue ("batteryCap", "Battery Capacity in mAh", batteryCap);
  cmd.AddValue ("voltage", "Battery voltage in Volts", voltage);
  cmd.AddValue ("txCurrent", "Tx current draw in mA", txCurrent);
  cmd.AddValue ("rxCurrent", "Rx current draw in mA", rxCurrent);
  cmd.AddValue ("sleepCurrent", "Sleep current draw in mA", sleepCurrent);
  cmd.AddValue ("numerology", "5G NR numerology", numerology);
  cmd.Parse (argc, argv);

  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
  //LogComponentEnable ("LteEnbRrc", LOG_LEVEL_ALL);
  //LogComponentEnable ("LteUeRrc", LOG_LEVEL_ALL);
  //LogComponentEnable ("EpcSgwPgwApplication", LOG_LEVEL_ALL);
  //LogComponentEnable ("MmWaveHelper" ThreeGppPropagationLossModel, LOG_LEVEL_INFO);
  //LogComponentEnable ("ThreeGppPropagationLossModel" , LOG_LEVEL_INFO);

  LogComponentEnableAll (LOG_PREFIX_FUNC);
  LogComponentEnableAll (LOG_PREFIX_NODE);
  LogComponentEnableAll (LOG_PREFIX_TIME);

  numUe = (int) (numUe / numEnb);
  distance = distance / numEnb;

  Config::SetDefault ("ns3::MmWaveHelper::RlcAmEnabled", BooleanValue (rlcAmEnabled));
  Config::SetDefault ("ns3::MmWaveHelper::HarqEnabled", BooleanValue (harqEnabled));
  Config::SetDefault ("ns3::MmWaveFlexTtiMacScheduler::HarqEnabled", BooleanValue (harqEnabled));
  Config::SetDefault ("ns3::LteRlcAm::ReportBufferStatusTimer", TimeValue (MicroSeconds (100.0)));
  Config::SetDefault ("ns3::LteRlcUmLowLat::ReportBufferStatusTimer", TimeValue (MicroSeconds (100.0)));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::Numerology", StringValue("NrNumerology"+numerology));

  Ptr<MmWaveHelper> mmwaveHelper = CreateObject<MmWaveHelper> ();
  mmwaveHelper->SetSchedulerType ("ns3::MmWaveFlexTtiMacScheduler");
  Ptr<MmWavePointToPointEpcHelper>  epcHelper = CreateObject<MmWavePointToPointEpcHelper> ();
  mmwaveHelper->SetEpcHelper (epcHelper);
  mmwaveHelper->SetHarqEnabled (harqEnabled);

  /*
  Propagation Loss Model association:
  - Urban: ThreeGppUmaPropagationLossModel (COST-HATA)
  - Suburban : ThreeGppRmaPropagationLossModel (LogDistance)
  - Rural: ThreeGppUmiStreetCanyonPropagationLossModel (Okumura-Hata)
  - Indoor: ThreeGppIndoorOfficePropagationLossModel (HybridBuildings)
  */
  std::string propLoss;
  if (radioEnvironment == "urban") propLoss = "ThreeGppUmaPropagationLossModel";
  else if (radioEnvironment == "suburban") propLoss = "ThreeGppRmaPropagationLossModel";
  else if (radioEnvironment == "rural") propLoss = "ThreeGppUmiStreetCanyonPropagationLossModel";
  else if (radioEnvironment == "indoor") propLoss = "ThreeGppIndoorOfficePropagationLossModel";

  mmwaveHelper->SetPathlossModelType ("ns3::"+propLoss);

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();

  // parse again so you can override default values from the command line
  cmd.Parse (argc, argv);

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("10000Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.0)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create (numEnb);
  ueNodes.Create (numUe);

  NodeContainer probingNode;

  //if (latency) {
    //probingNode.Create (1);
  //}

  // Install Mobility Model
  MobilityHelper mobility;
  if (radioEnvironment == "indoor") {
      double x_min = 0.0;
      double x_max = 20.0;
      double y_min = 0.0;
      double y_max = 10.0;
      double z_min = 0.0;
      double z_max = 50.0;

      double _x_max = x_max / numEnb;
      double _y_max = y_max / numEnb;
      double _z_max = z_max / numEnb;

      std::string m_x = "ns3::UniformRandomVariable[Min=0.0|Max="+std::to_string(_x_max)+"]";
      std::string m_y = "ns3::UniformRandomVariable[Min=0.0|Max="+std::to_string(_y_max)+"]";
      std::string m_z = "ns3::UniformRandomVariable[Min=0.0|Max="+std::to_string(_z_max)+"]";

      mobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator", "X", StringValue (m_x),
                                    "Y", StringValue (m_y), "Z", StringValue (m_z));
      mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobility.Install(ueNodes);

      MobilityHelper mobilityAp;
      Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
      positionAlloc->Add (Vector (_x_max/2, _y_max/2, _z_max/2));
      mobilityAp.SetPositionAllocator (positionAlloc);
      mobilityAp.SetMobilityModel("ns3::ConstantPositionMobilityModel");
      mobilityAp.Install(enbNodes);

      //double z_max = 50.0;
      Ptr<Building> b = CreateObject <Building> ();
      b->SetBoundaries (Box (x_min, x_max, y_min, y_max, z_min, z_max));
      b->SetBuildingType (Building::Residential);
      b->SetExtWallsType (Building::ConcreteWithWindows);
      b->SetNFloors (16);
      b->SetNRoomsX (3);
      b->SetNRoomsY (2);

      BuildingsHelper::Install (ueNodes);
      
      BuildingsHelper::Install (enbNodes);

      if (latency) {
        positionAlloc->Add (Vector (x_max/2 + 1, y_max/2, z_max/2));
        mobility.SetPositionAllocator (positionAlloc);
        mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
        mobility.Install (probingNode);
        BuildingsHelper::Install (probingNode);
      }
    }
    else {
      mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator", "rho", DoubleValue (distance),
                                  "X", DoubleValue (0.0), "Y", DoubleValue (0.0), "Z", DoubleValue(1.0));
      mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobility.Install(ueNodes);

      MobilityHelper mobilityAp;
      Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
      positionAlloc->Add (Vector (0, 0, 1));
      mobilityAp.SetPositionAllocator (positionAlloc);
      mobilityAp.SetMobilityModel("ns3::ConstantPositionMobilityModel");
      mobilityAp.Install(enbNodes);

      if (latency) {
        positionAlloc->Add (Vector (distance, 0, 1));
        mobility.SetPositionAllocator (positionAlloc);
        mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
        mobility.Install (probingNode);
      }
    }  

  // Install mmWave Devices to the nodes
  NetDeviceContainer enbmmWaveDevs = mmwaveHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer uemmWaveDevs = mmwaveHelper->InstallUeDevice (ueNodes);

  NetDeviceContainer probingmmWaveDevs;

  //if (latency) {
    //probingmmWaveDevs = mmwaveHelper->InstallUeDevice (probingNode);
  //}

  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  if (latency) {
    internet.Install (probingNode);
  }
  Ipv4InterfaceContainer ueIpIface, probingIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (uemmWaveDevs));
  if (latency) {
    probingIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (probingmmWaveDevs));
  }
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }
  
  /*
  if (latency) {
    Ptr<Node> probingNodePtr = probingNode.Get (0);
    // Set the default gateway for the UE
    Ptr<Ipv4StaticRouting> probingStaticRouting = ipv4RoutingHelper.GetStaticRouting (probingNodePtr->GetObject<Ipv4> ());
    probingStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
  }
  */

  mmwaveHelper->AttachToClosestEnb (uemmWaveDevs, enbmmWaveDevs);

  //if (latency) {
    //mmwaveHelper->AttachToClosestEnb (probingmmWaveDevs, enbmmWaveDevs);
  //}

  
  if (latency) {
    // UDP Echo Server application to be installed in the AP
    int echoPort = 11;
    UdpEchoServerHelper echoServer(echoPort); // Port # 9
    uint32_t payloadSizeEcho = 100; //Packet size for Echo UDP App

    if (trafficDirection == "upstream") {
      ApplicationContainer serverApps = echoServer.Install(remoteHost);
      serverApps.Start(Seconds(0.0));
      serverApps.Stop(Seconds(simulationTime+1));
      
      //wifiApInterface.GetAddress(0).Print(std::cout);
      // UDP Echo Client application to be installed in the probing station
      UdpEchoClientHelper echoClient1(remoteHostAddr, echoPort);
      
      echoClient1.SetAttribute("MaxPackets", UintegerValue(10000));
      echoClient1.SetAttribute("Interval", TimeValue(Seconds(1)));
      echoClient1.SetAttribute("PacketSize", UintegerValue(payloadSizeEcho));

      ApplicationContainer clientApp = echoClient1.Install(ueNodes.Get(0));
      //commInterfaces.GetAddress(0).Print(std::cout);
      clientApp.Start(Seconds(1.0));
      clientApp.Stop(Seconds(simulationTime+1));
    }
    else {
      ApplicationContainer serverApps = echoServer.Install(probingNode);
      serverApps.Start(Seconds(0.0));
      serverApps.Stop(Seconds(simulationTime+1));

      // UDP Echo Client application to be installed in the probing station
      UdpEchoClientHelper echoClient1(probingIpIface.GetAddress(0), echoPort);
      
      echoClient1.SetAttribute("MaxPackets", UintegerValue(10000));
      echoClient1.SetAttribute("Interval", TimeValue(Seconds(2)));
      echoClient1.SetAttribute("PacketSize", UintegerValue(payloadSizeEcho));

      ApplicationContainer clientApps = echoClient1.Install(remoteHost);
      //commInterfaces.GetAddress(0).Print(std::cout);
      clientApps.Start(Seconds(1.0));
      clientApps.Stop(Seconds(simulationTime+1));
    }
  }
  

  // Install and start applications on UEs and remote host
  uint16_t dlPort = 1234;
  uint16_t ulPort = 2000;
  uint16_t otherPort = 3000;
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
  double min = 0.5;
  double max = 1.0;
  PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
  serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
  for (uint32_t u = 1; u < ueNodes.GetN (); ++u)
    {
      //PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
      //PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort));
      //serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get (u)));
      
      //serverApps.Add (packetSinkHelper.Install (ueNodes.Get (u)));

      UdpClientHelper ulClient (remoteHostAddr, ulPort);
      ulClient.SetAttribute ("Interval", TimeValue (Seconds (interPacketInterval)));
      ulClient.SetAttribute ("MaxPackets", UintegerValue (1000000));
      ulClient.SetAttribute("PacketSize", UintegerValue(payloadSize));

      // Desynchronize the sending applications
      Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
      x->SetAttribute ("Min", DoubleValue (min));
      x->SetAttribute ("Max", DoubleValue (max));

      double value = 1 + x->GetValue ();

      clientApps = ulClient.Install (ueNodes.Get(u));
      clientApps.Start(Seconds(value));
      clientApps.Stop(Seconds(simulationTime+value));
    }

  serverApps.Start (Seconds (0.0));
  //clientApps.Start (Seconds (0.1));
  //mmwaveHelper->EnableTraces ();
  // Uncomment to enable PCAP tracing
  //p2ph.EnablePcapAll ("mmwave-epc-simple");

  // Energy Framework
  BasicEnergySourceHelper basicSourceHelper;
  basicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (1000000));
  basicSourceHelper.Set ("BasicEnergySupplyVoltageV", DoubleValue (3.0));
  // Install Energy Source
  EnergySourceContainer sources = basicSourceHelper.Install (ueNodes);
  EnergySourceContainer Enb_sources = basicSourceHelper.Install (enbNodes);
  // Device Energy Model
  MmWaveRadioEnergyModelHelper nrEnergyHelper;
  MmWaveRadioEnergyModelEnbHelper enbEnergyHelper;

  nrEnergyHelper.Set ("SleepA", DoubleValue (sleepCurrent/1000));
  nrEnergyHelper.Set ("TxCurrentA", DoubleValue (txCurrent/1000));
  nrEnergyHelper.Set ("RxCurrentA", DoubleValue (rxCurrent/1000));

  DeviceEnergyModelContainer deviceEnergyModel = nrEnergyHelper.Install (uemmWaveDevs, sources);
  DeviceEnergyModelContainer bsEnergyModel = enbEnergyHelper.Install (enbmmWaveDevs, Enb_sources);

  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();

  double totalSent = 0;
    for (uint32_t index = 0; index < clientApps.GetN (); ++index) {
      totalSent += DynamicCast<UdpClient> (clientApps.Get (index)) ->GetTotalTx ();
    }

    totalSent *= (numUe-1);

    double totalPacketsThrough = 0, throughput = 0;
  for (uint32_t index = 0; index < serverApps.GetN (); ++index) {
      /*/ Calculating Packet throughput and packet delivery /*/
      totalPacketsThrough += DynamicCast<PacketSink> (serverApps.Get (index)) ->GetTotalRx ();
      throughput += ((totalPacketsThrough * 8) / ((simulationTime) * 1024)); //Mbit/s
    }
  
  double capacityJoules = batteryCap * voltage * 3.6;

  double max_energy = 0, max_battery_lifetime = 999999;

  for (DeviceEnergyModelContainer::Iterator iter = deviceEnergyModel.Begin (); iter != deviceEnergyModel.End (); iter ++) {
      double energyConsumed = (*iter)->GetTotalEnergyConsumption (); 
      max_energy = std::max(energyConsumed, max_energy);

      double battery_lifetime = ((capacityJoules / energyConsumed) * simulationTime);
      battery_lifetime = battery_lifetime / 86400; // Days
      max_battery_lifetime = std::min(battery_lifetime, max_battery_lifetime);      
      //break; // Energy in only one station
  }

  std::cout << "Total energy consumed by radio (Station): " << max_energy << std::endl;
  std::cout << "Battery lifetime: " << max_battery_lifetime << std::endl;
  
  std::cout << "Throughput: " << throughput << std::endl;
  double wholeSuccessRate =  (totalPacketsThrough / totalSent) * 100;
  std::cout << "Success rate: " <<  wholeSuccessRate << " " << totalPacketsThrough << " " << totalSent << std::endl; // Success rate percentage

  Simulator::Destroy ();
  return 0;

}
