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

using namespace ns3;

int main (int argc, char** argv)
{
  SeedManager::SetSeed (3);  // Changes seed from default of 1 to 3
  SeedManager::SetRun (1);  // Changes run number from default of 1 to 7

  bool verbose = true;
  bool disablePcap = true;
  bool disableAsciiTrace = true;

  int nDevices = 7; // 1 PAN Node + 1 Probing node + (nDevices-2) communication nodes
  int simulationTime = 20;
  uint8_t max_BE = 4;
  uint8_t min_BE = 2;
  uint8_t csma_backoffs = 6;
  uint8_t maxFrameRetries = 5;
  int distance = 1;
  std::string propLoss = "LogDistancePropagationModel";

  CommandLine cmd (__FILE__);
  cmd.AddValue ("verbose", "turn on log components", verbose);
  cmd.AddValue ("disable-pcap", "disable PCAP generation", disablePcap);
  cmd.AddValue ("disable-asciitrace", "disable ascii trace generation", disableAsciiTrace);
  cmd.AddValue ("nDevices", "number of stations", nDevices);
  cmd.Parse (argc, argv);
  
  if (verbose)
    {
      //LogComponentEnable ("LrWpanMac", LOG_LEVEL_ALL);
      //LogComponentEnable ("LrWpanPhy", LOG_LEVEL_ALL);
      //LogComponentEnable ("LrWpanNetDevice", LOG_LEVEL_ALL);
      //LogComponentEnable ("SixLowPanNetDevice", LOG_LEVEL_ALL);
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);
      LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
      LogComponentEnable ("LrWpanMac", LOG_LEVEL_INFO);
      LogComponentEnable ("LrWpanPhy", LOG_LEVEL_INFO);
      LogComponentEnableAll (LOG_PREFIX_FUNC);
      LogComponentEnableAll (LOG_PREFIX_NODE);
      LogComponentEnableAll (LOG_PREFIX_TIME);
    }

  NodeContainer nodes;
  nodes.Create(nDevices);

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

  for (uint32_t i = 0; i < nDevices; i++) {
      positionAlloc->Add (Vector (distance, 0.0, 0.0));
  }

  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);
  
  LrWpanHelper lrWpanHelper;
  // Add and install the LrWpanNetDevice for each node
  NetDeviceContainer lrwpanDevices = lrWpanHelper.Install(nodes);

  // Fake PAN association and short address assignment.
  // This is needed because the lr-wpan module does not provide (yet)
  // a full PAN association procedure.
  lrWpanHelper.AssociateToPan (lrwpanDevices, 0, max_BE, min_BE, csma_backoffs, maxFrameRetries, propLoss);

  InternetStackHelper internetv6;
  internetv6.Install (nodes);

  SixLowPanHelper sixlowpan;
  NetDeviceContainer devices = sixlowpan.Install (lrwpanDevices); 
 
  Ipv6AddressHelper ipv6;
  ipv6.SetBase (Ipv6Address ("2001:2::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer deviceInterfaces;
  deviceInterfaces = ipv6.Assign (devices);
  Ipv6Address panAddr = deviceInterfaces.GetAddress (0,1);
   
  uint32_t packetSizeEcho = 40;
  uint32_t maxPacketCount = 200000;
  Time interPacketInterval = Seconds (2.);

  uint16_t port = 11, echoPort = 10; // well-known echo port number
  
  UdpServerHelper echoServer(echoPort); // Port # 10

  ApplicationContainer echoServerApps = echoServer.Install(nodes.Get(0));
  echoServerApps.Start(Seconds(0.0));
  echoServerApps.Stop(Seconds(simulationTime+1));
  
  // UDP Echo Client application to be installed in the probing station
  Address serverAddress = Address(deviceInterfaces.GetAddress (0, 1));
  UdpEchoClientHelper echoClient1(serverAddress, echoPort);
  
  echoClient1.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
  echoClient1.SetAttribute("Interval", TimeValue(interPacketInterval));
  echoClient1.SetAttribute("PacketSize", UintegerValue(packetSizeEcho));

  ApplicationContainer clientApps1 = echoClient1.Install(nodes.Get(1));
  clientApps1.Start(Seconds(1.0));
  clientApps1.Stop(Seconds(simulationTime+1));
  
  uint32_t packetSize = 500; //Bytes

  Inet6SocketAddress socket = Inet6SocketAddress (panAddr, port);
  PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", socket);
  ApplicationContainer serverApps, clientApps;
  serverApps.Add (ulPacketSinkHelper.Install (nodes.Get(0)));

  double min = 0.0;
  double max = 0.5;

  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();

  for (uint32_t index = 2; index < nDevices; ++index) {
    OnOffHelper client ("ns3::UdpSocketFactory", socket);
    client.SetAttribute ("PacketSize", UintegerValue (packetSize));
    client.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    client.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    client.SetAttribute ("DataRate", DataRateValue (DataRate ("90Kbps"))); //Value greater than maximum possible value

    double value = x->GetValue (0, 10);

    clientApps = client.Install (nodes.Get(index));
    clientApps.Start (Seconds (value));
    clientApps.Stop (Seconds (simulationTime+value));
  }

  serverApps.Start (Seconds (0));
  serverApps.Stop(Seconds (simulationTime+1));

 DeviceEnergyModelContainer deviceModels;

  double voltage = 3;
  double capacity = 2300; // 5.2 Ah * 12 V * 3600

   double capacityJoules = capacity * voltage * 3.6; // 5.2 Ah * 12 V * 3600

  LrWpanEnergySourceHelper basicSourceHelper;
  EnergySourceContainer sources = basicSourceHelper.Install(nodes);

  LrWpanRadioEnergyModelHelper radioEnergyHelper;
  deviceModels = radioEnergyHelper.Install (lrwpanDevices, sources);

  if (!disableAsciiTrace)
    {
      AsciiTraceHelper ascii;
      lrWpanHelper.EnableAsciiAll (ascii.CreateFileStream ("cbr.tr"));
    }
  if (!disablePcap)
    {
      lrWpanHelper.EnablePcapAll ("cbr", devices.Get(0));
    }

  Simulator::Stop (Seconds (simulationTime+1));
  
  Simulator::Run ();

  double energy = 0;
  for (DeviceEnergyModelContainer::Iterator iter = deviceModels.Begin (); iter != deviceModels.End (); iter ++) {
    double energyConsumed = (*iter)->GetTotalEnergyConsumption ();
    NS_LOG_UNCOND ("End of simulation (" << Simulator::Now ().GetSeconds ()
                  << "s) Total energy consumed by radio (Station) = " << energyConsumed << "J");
    energy = energyConsumed;
    std::cout << "Battery lifetime: " << ((capacityJoules / energyConsumed) * simulationTime) / 86400 << std::endl;
  }

  UintegerValue packetsSent;
  double totalpacketsSent = 0;
  for (uint32_t index = 0; index < clientApps.GetN (); ++index) {
    clientApps.Get (index)->GetAttribute("TotalTx", packetsSent);    
    totalpacketsSent = totalpacketsSent + packetsSent.Get();
  }

  double totalPacketsThrough, throughput;
  
  for (uint32_t index = 0; index < serverApps.GetN (); ++index) {
      /*/ Calculating Packet throughput and packet delivery /*/
      totalPacketsThrough = DynamicCast<PacketSink> (
                            serverApps.Get (0))->GetTotalRx ();
      throughput += ((totalPacketsThrough * 8) / ((simulationTime) * 1024.0)); //Kbps
      std::cout << "Packet Throughput: " << throughput << std::endl;
      double successRate =  (totalPacketsThrough / totalpacketsSent / (nDevices-2)) * 100;
      std::cout << "Packet Delivery: " << successRate << std::endl; // %
  }
}