/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Universita' di Firenze, Italy
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/mobility-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/propagation-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/applications-module.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/random-variable-stream.h"
#include "ns3/lr-wpan-radio-energy-model.h"
#include "ns3/energy-module.h"
#include "ns3/core-module.h"
#include "ns3/buildings-helper.h"
#include "ns3/building.h"

using namespace ns3;

int main (int argc, char** argv)
{
  SeedManager::SetSeed (3);  // Changes seed from default of 1 to 3
  SeedManager::SetRun (7);  // Changes run number from default of 1 to 7

  bool verbose = true;
  bool disablePcap = true;
  bool disableAsciiTrace = false;

  double nSta = 20; // 1 PAN Node + 1 Probing node + (nDevices-2) communication nodes
  double nGW = 4; // 1 PAN Node + 1 Probing node + (nDevices-2) communication nodes
  double simulationTime = 100;
  double distance = 50;
  double period = 1;
  uint32_t packetSize = 20; //Bytes
  std::string trafficDirection = "upstream";
  double voltage = 3;
  double capacity = 2400;
  uint8_t min_BE = 3;
  uint8_t max_BE = 5;
  uint8_t csma_backoffs = 4;
  uint8_t maxFrameRetries = 5;
  // Topology file name
  std::string topologyFile = "topology.csv";
  std::string radioEnvironment = "indoor";

  bool latency = true;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("nSta", "Number of stations", nSta);
  cmd.AddValue ("nGW", "Number of stations", nGW);
  cmd.AddValue ("distance", "Distance between EDs and PAN", distance);
  cmd.AddValue ("radioEnvironment", "Loss Propagation Model", radioEnvironment);
  cmd.AddValue ("simulationTime", "Simulation time", simulationTime);
  cmd.AddValue ("period", "Packet period", period);
  cmd.AddValue ("max_BE", "Max Backoff Exponent", max_BE);
  cmd.AddValue ("min_BE", "Min Backoff Exponent", min_BE);
  cmd.AddValue ("csma_backoffs", "CSMA Backoffs", csma_backoffs);
  cmd.AddValue ("maxFrameRetries", "Max Frame Retries", maxFrameRetries);
  cmd.AddValue ("packetSize", "Packet size", packetSize);
  cmd.AddValue ("trafficDirection", "Traffic direction", trafficDirection);
  cmd.AddValue ("voltage", "Voltage capacity", voltage);
  cmd.AddValue ("capacity", "Batttery capacity", capacity);
  cmd.AddValue ("topologyFile", "Topology file name", topologyFile);
  cmd.Parse (argc, argv);
  
  if (verbose)
    {
      //LogComponentEnable ("LrWpanMac", LOG_LEVEL_ALL);
      //LogComponentEnable ("LrWpanPhy", LOG_LEVEL_ALL);
      //LogComponentEnable ("LrWpanNetDevice", LOG_LEVEL_ALL);
      //LogComponentEnable ("SixLowPanNetDevice", LOG_LEVEL_ALL);
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
      LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
      LogComponentEnableAll (LOG_PREFIX_FUNC);
      LogComponentEnableAll (LOG_PREFIX_NODE);
      LogComponentEnableAll (LOG_PREFIX_TIME);
    }
  //nSta = nSta + 2; // Add GW and probing node
  nSta = (int) (nSta / nGW);
  nSta = nSta + 2;
  distance = distance / nGW;

  //std::cout << nSta << " " << distance << std::endl;

  NodeContainer nodes;
  nodes.Create(nSta);

  MobilityHelper mobility;  

  // Setting stations' positions
  if (radioEnvironment == "indoor") {
      double x_min = 0.0;
      double x_max = 20.0;
      double y_min = 0.0;
      double y_max = 10.0;
      double z_min = 0.0;
      double z_max = 50.0;

      double _x_max = x_max / nGW;
      double _y_max = y_max / nGW;
      double _z_max = z_max / nGW;

      std::string m_x = "ns3::UniformRandomVariable[Min=0.0|Max="+std::to_string(_x_max)+"]";
      std::string m_y = "ns3::UniformRandomVariable[Min=0.0|Max="+std::to_string(_y_max)+"]";
      std::string m_z = "ns3::UniformRandomVariable[Min=0.0|Max="+std::to_string(_z_max)+"]";

      mobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator", "X", StringValue (m_x),
                                    "Y", StringValue (m_y), "Z", StringValue (m_z));
      mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobility.Install(nodes);

      MobilityHelper mobilityAp;
      Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
      positionAlloc->Add (Vector (_x_max/2, _y_max/2, _z_max/2));
      mobilityAp.SetPositionAllocator (positionAlloc);
      mobilityAp.SetMobilityModel("ns3::ConstantPositionMobilityModel");
      mobilityAp.Install(nodes.Get(1));

      //std::cout << z_max << std::endl;
      //double z_max = 50.0;
      Ptr<Building> b = CreateObject <Building> ();
      b->SetBoundaries (Box (x_min, x_max, y_min, y_max, z_min, z_max));
      b->SetBuildingType (Building::Residential);
      b->SetExtWallsType (Building::ConcreteWithWindows);
      b->SetNFloors (16);
      b->SetNRoomsX (3);
      b->SetNRoomsY (2);

      BuildingsHelper::Install (nodes);
    
      if (latency) {
        positionAlloc->Add (Vector (x_max/2 + 1, y_max/2, z_max/2));
        mobility.SetPositionAllocator (positionAlloc);
        mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
        mobility.Install (nodes.Get(2));
    }
    }
    else {
      mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator", "rho", DoubleValue (distance),
                                  "X", DoubleValue (0.0), "Y", DoubleValue (0.0), "Z", DoubleValue(1.0));
      mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobility.Install(nodes);

      MobilityHelper mobilityAp;
      Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
      positionAlloc->Add (Vector (0, 0, 1));
      mobilityAp.SetPositionAllocator (positionAlloc);
      mobilityAp.SetMobilityModel("ns3::ConstantPositionMobilityModel");
      mobilityAp.Install(nodes.Get(1));

      if (latency) {
        positionAlloc->Add (Vector (distance, 0, 1));
        mobility.SetPositionAllocator (positionAlloc);
        mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
        mobility.Install (nodes.Get(2));
    }
    }

  /*
  NodeContainer gwNodes;
  gwNodes.Create(nGW);
  NodeContainer probingNode;

  if (latency) {
    probingNode.Create(1);
  }    

  CsvReader csv (topologyFile);


  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  Ptr<ListPositionAllocator> positionAp = CreateObject<ListPositionAllocator> ();

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
      mobility.Install (staNodes);

      for (uint32_t i = 0; i < nGW; i++) {
        csv.FetchNextRow ();
        ok1 = csv.GetValue (0, x);
        ok2= csv.GetValue (1, y);
        ok3 = csv.GetValue (2, z);
        positionAp->Add (Vector (x, y, z));
        //std::cout << x << " AP and " << y << " and " << z << std::endl;
      }
    
      mobility.SetPositionAllocator (positionAp);
      mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobility.Install (gwNodes);

      break;
    }
  
  */
  
  LrWpanHelper lrWpanHelper;
  // Add and install the LrWpanNetDevice for each node
  NetDeviceContainer lrwpanDevices = lrWpanHelper.Install(nodes);

  /*
  NetDeviceContainer lrwpanGWDevices = lrWpanHelper.Install(gwNodes);
  NetDeviceContainer lrwpanProbingDevice;

  if (latency) {
    lrwpanProbingDevice = lrWpanHelper.Install(probingNode);
  }
  */

  // Fake PAN association and short address assignment.
  // This is needed because the lr-wpan module does not provide (yet)
  // a full PAN association procedure.
  lrWpanHelper.AssociateToPan (lrwpanDevices, 0, max_BE, min_BE, csma_backoffs, maxFrameRetries, radioEnvironment);

  InternetStackHelper internetv6;
  internetv6.Install (nodes);

  //if (latency) internetv6.Install (probingNode);

  SixLowPanHelper sixlowpan;
  NetDeviceContainer devices = sixlowpan.Install (lrwpanDevices);
 
  Ipv6AddressHelper ipv6;
  ipv6.SetBase (Ipv6Address ("2001:2::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer interfaces;
  interfaces = ipv6.Assign (devices);
  //Ipv6InterfaceContainer gwInterfaces;
  //gwInterfaces = ipv6.Assign (gwDevices);
   
  uint32_t packetSizeEcho = 50;
  uint32_t maxPacketCount = 200000;
  Time echoInterPacketInterval = Seconds (2.), interPacketInterval = Seconds (period);

  uint16_t port = 20, echoPort = 10; // well-known echo port number
  
  UdpEchoServerHelper echoServer(echoPort); // Port # 10
  ApplicationContainer echoServerApps = echoServer.Install(nodes.Get(0));
  echoServerApps.Start(Seconds(0.0));
  echoServerApps.Stop(Seconds(simulationTime+1));

  // UDP Echo Client application to be installed in the probing station
  Address serverAddress = Address(interfaces.GetAddress (0, 1));
  UdpEchoClientHelper echoClient1(serverAddress, echoPort);
    
  echoClient1.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
  echoClient1.SetAttribute("Interval", TimeValue(echoInterPacketInterval));
  echoClient1.SetAttribute("PacketSize", UintegerValue(packetSizeEcho));

  ApplicationContainer clientApps1 = echoClient1.Install(nodes.Get(1));
  clientApps1.Start(Seconds(0.0));
  clientApps1.Stop(Seconds(simulationTime+1));
  
  double min = 1.0;
  double max = 0.5;

  ApplicationContainer serverApps, clientApps;

  Ipv6Address panAddr = interfaces.GetAddress (0,1);
  Inet6SocketAddress socket = Inet6SocketAddress (panAddr, port);
  PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", socket);
  serverApps.Add (ulPacketSinkHelper.Install (nodes.Get(0)));
  serverApps.Start (Seconds (0));
  serverApps.Stop (Seconds (simulationTime+period));

  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();

  for (uint32_t index = 2; index < nSta; ++index) {
    /*
    csv.FetchNextRow ();
    ok = csv.GetValue (0, val);

    Address serverAddress = Address(gwInterfaces.GetAddress (val, 1));
    */

    Address serverAddress = Address(interfaces.GetAddress (0, 1));
    UdpClientHelper client (serverAddress, port);
    client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
    client.SetAttribute ("Interval", TimeValue (interPacketInterval));
    client.SetAttribute ("PacketSize", UintegerValue (packetSize));

    double value = x->GetValue (0, period);

    //double value = 3;

    //std::cout << value << std::endl;

    clientApps = client.Install (nodes.Get (index));
    
    clientApps.Start (Seconds (value));
    clientApps.Stop (Seconds (simulationTime+value));
  }

  DeviceEnergyModelContainer deviceModels;

  
  double capacityJoules = capacity * voltage * 3.6; // 5.2 Ah * 12 V * 3600

  //BasicEnergySourceHelper basicSourceHelper;
  LrWpanEnergySourceHelper basicSourceHelper;
  EnergySourceContainer sources = basicSourceHelper.Install(nodes);

  LrWpanRadioEnergyModelHelper radioEnergyHelper;
  deviceModels = radioEnergyHelper.Install (lrwpanDevices, sources);

  if (!disableAsciiTrace)
    {
      AsciiTraceHelper ascii;
      lrWpanHelper.EnableAsciiAll (ascii.CreateFileStream ("telemetry.tr"));
    }
  if (!disablePcap)
    {
      lrWpanHelper.EnablePcapAll ("telemetry", devices.Get(0));
    }

  Simulator::Stop (Seconds (simulationTime+period));
  
  Simulator::Run ();

  double max_energy = 0, max_battery_lifetime = 999999;
  for (DeviceEnergyModelContainer::Iterator iter = deviceModels.Begin (); iter != deviceModels.End (); iter ++) {
    double energyConsumed = (*iter)->GetTotalEnergyConsumption (); 
    max_energy = std::max(energyConsumed, max_energy);
    double battery_lifetime = ((capacityJoules / energyConsumed) * simulationTime);

    battery_lifetime = battery_lifetime / 86400; // Days
    max_battery_lifetime = std::min(battery_lifetime, max_battery_lifetime);
  }
  std::cout << "Total energy consumed by radio (Station): " << max_energy << std::endl;
  std::cout << "Battery lifetime: " << ((capacityJoules / max_energy) * simulationTime) / 86400 << std::endl;

  double totalPacketsThrough, throughput;
  for (uint32_t index = 0; index < serverApps.GetN (); ++index) {
    totalPacketsThrough = DynamicCast<PacketSink> (serverApps.Get (index)) ->GetTotalRx ();
    throughput += ((totalPacketsThrough * 8) / ((simulationTime) * 1024)); //Kbit/s
    std::cout << "Throughput: " << throughput << std::endl;
  }

  double totalSent;
  if (trafficDirection == "upstream") {
    for (uint32_t index = 0; index < clientApps.GetN (); ++index) {
      totalSent += DynamicCast<UdpClient> (clientApps.Get (index)) ->GetTotalTx ();
    }
  }
  totalSent *= (nSta-2);
  
  double wholeSuccessRate =  (totalPacketsThrough / totalSent) * 100;
  wholeSuccessRate = std::min(100.0, wholeSuccessRate);
  //std::cout << totalPacketsThrough << " " << totalSent << std::endl; // Success rate percentage
  std::cout << "Success rate: " <<  wholeSuccessRate << std::endl; // Success rate percentage

  Simulator::Destroy ();
}