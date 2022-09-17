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

using namespace ns3;
using namespace mmwave;

/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,
 * attaches one UE per eNodeB starts a flow for each UE to  and from a remote host.
 * It also  starts yet another flow between each UE pair.
 */
NS_LOG_COMPONENT_DEFINE ("EpcFirstExample");
int
main (int argc, char *argv[])
{
  uint16_t numEnb = 1;
  uint16_t numUe = 1;
  double simTime = 2.0;
  double interPacketInterval = 1;
  double minDistance = 10.0; // eNB-UE distance in meters
  double maxDistance = 150.0; // eNB-UE distance in meters
  bool harqEnabled = true;
  bool rlcAmEnabled = false;
  std::string dataRate = "50";

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue ("numEnb", "Number of eNBs", numEnb);
  cmd.AddValue ("numUe", "Number of UEs per eNB", numUe);
  cmd.AddValue ("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue ("interPacketInterval", "Inter-packet interval [us])", interPacketInterval);
  cmd.AddValue ("harq", "Enable Hybrid ARQ", harqEnabled);
  cmd.AddValue ("rlcAm", "Enable RLC-AM", rlcAmEnabled);
  cmd.Parse (argc, argv);

  LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
  LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);

  LogComponentEnableAll (LOG_PREFIX_FUNC);
  LogComponentEnableAll (LOG_PREFIX_NODE);
  LogComponentEnableAll (LOG_PREFIX_TIME);

  Config::SetDefault ("ns3::MmWaveHelper::RlcAmEnabled", BooleanValue (rlcAmEnabled));
  Config::SetDefault ("ns3::MmWaveHelper::HarqEnabled", BooleanValue (harqEnabled));
  Config::SetDefault ("ns3::MmWaveFlexTtiMacScheduler::HarqEnabled", BooleanValue (harqEnabled));
  Config::SetDefault ("ns3::LteRlcAm::ReportBufferStatusTimer", TimeValue (MicroSeconds (100.0)));
  Config::SetDefault ("ns3::LteRlcUmLowLat::ReportBufferStatusTimer", TimeValue (MicroSeconds (100.0)));

  Ptr<MmWaveHelper> mmwaveHelper = CreateObject<MmWaveHelper> ();
  mmwaveHelper->SetSchedulerType ("ns3::MmWaveFlexTtiMacScheduler");
  Ptr<MmWavePointToPointEpcHelper>  epcHelper = CreateObject<MmWavePointToPointEpcHelper> ();
  mmwaveHelper->SetEpcHelper (epcHelper);
  mmwaveHelper->SetHarqEnabled (harqEnabled);

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
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
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

  // Install Mobility Model
  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  enbPositionAlloc->Add (Vector (0.0, 0.0, 0.0));
  MobilityHelper enbmobility;
  enbmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbmobility.SetPositionAllocator (enbPositionAlloc);
  enbmobility.Install (enbNodes);

  MobilityHelper uemobility;
  Ptr<ListPositionAllocator> uePositionAlloc = CreateObject<ListPositionAllocator> ();
  Ptr<UniformRandomVariable> distRv = CreateObject<UniformRandomVariable> ();
  for (unsigned i = 0; i < numUe; i++)
    {
      double dist = distRv->GetValue (minDistance, maxDistance);
      uePositionAlloc->Add (Vector (dist, 0.0, 0.0));
    }
  uemobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  uemobility.SetPositionAllocator (uePositionAlloc);
  uemobility.Install (ueNodes);

  // Install mmWave Devices to the nodes
  NetDeviceContainer enbmmWaveDevs = mmwaveHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer uemmWaveDevs = mmwaveHelper->InstallUeDevice (ueNodes);

  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface, enbIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (uemmWaveDevs));
  enbIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (enbmmWaveDevs));
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  mmwaveHelper->AttachToClosestEnb (uemmWaveDevs, enbmmWaveDevs);


  // Install and start applications on UEs and remote host
  uint16_t dlPort = 1234;
  uint16_t ulPort = 2000;
  uint16_t otherPort = 3000;
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      ++ulPort;
      ++otherPort;
      PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
      PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
      PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort));
      //serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get (u)));
      serverApps.Add (ulPacketSinkHelper.Install (enbNodes.Get(u)));
      //serverApps.Add (packetSinkHelper.Install (ueNodes.Get (u)));

      //auto ipv4 = wifiApNode.Get (0)->GetObject<Ipv4> ();
      //const auto address = ipv4->GetAddress (1, 0).GetLocal ();
      /*
      InetSocketAddress dlSinkSocket (ueIpIface.GetAddress (u), dlPort);

      OnOffHelper dlClient ("ns3::UdpSocketFactory", dlSinkSocket);
      dlClient.SetAttribute ("PacketSize", UintegerValue (1000));
      dlClient.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
      dlClient.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      dlClient.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
      */

      InetSocketAddress ulSinkSocket (enbIpIface.GetAddress (0), dlPort);

      OnOffHelper ulClient ("ns3::UdpSocketFactory", ulSinkSocket);
      ulClient.SetAttribute ("PacketSize", UintegerValue (1024));
      ulClient.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
      ulClient.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      ulClient.SetAttribute ("DataRate", DataRateValue (DataRate ("50Mbps")));

//      UdpClientHelper client (ueIpIface.GetAddress (u), otherPort);
//      client.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
//      client.SetAttribute ("MaxPackets", UintegerValue(1000000));

      //clientApps.Add (dlClient.Install (remoteHost));
      clientApps.Add (ulClient.Install (ueNodes.Get(u)));
//      if (u+1 < ueNodes.GetN ())
//        {
//          clientApps.Add (client.Install (ueNodes.Get(u+1)));
//        }
//      else
//        {
//          clientApps.Add (client.Install (ueNodes.Get(0)));
//        }
    }

  serverApps.Start (Seconds (0.1));
  clientApps.Start (Seconds (0.1));
  mmwaveHelper->EnableTraces ();
  // Uncomment to enable PCAP tracing
  p2ph.EnablePcapAll ("mmwave-epc-simple");

  // Energy Framework
  BasicEnergySourceHelper basicSourceHelper;
  basicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (1000000));
  basicSourceHelper.Set ("BasicEnergySupplyVoltageV", DoubleValue (5.0));
  // Install Energy Source
  EnergySourceContainer sources = basicSourceHelper.Install (ueNodes);
  EnergySourceContainer Enb_sources = basicSourceHelper.Install (enbNodes);
  // Device Energy Model
  MmWaveRadioEnergyModelHelper nrEnergyHelper;
  MmWaveRadioEnergyModelEnbHelper enbEnergyHelper;
  DeviceEnergyModelContainer deviceEnergyModel = nrEnergyHelper.Install (uemmWaveDevs, sources);
  DeviceEnergyModelContainer bsEnergyModel = enbEnergyHelper.Install (enbmmWaveDevs, Enb_sources);

  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();

  UintegerValue packetsSent;
  double totalpacketsSent = 0;
  for (uint32_t index = 0; index < clientApps.GetN (); ++index) {
    clientApps.Get (index)->GetAttribute("TotalTx", packetsSent);    
    totalpacketsSent = totalpacketsSent + packetsSent.Get();
  }

  double throughput = 0, successRate;
  for (uint32_t index = 0; index < serverApps.GetN (); ++index) {
      /*/ Calculating Packet throughput and packet delivery /*/
      double totalPacketsThrough = DynamicCast<PacketSink> (
                            serverApps.Get (0))->GetTotalRx ();
      throughput += ((totalPacketsThrough * 8) / ((simTime) * 1000000.0)); //Mbps
      successRate =  (totalPacketsThrough / totalpacketsSent / ueNodes.GetN()) * 100;
    }
    std::cout << "Packet Delivery: " << successRate << std::endl; // %

  double energy;
  for (DeviceEnergyModelContainer::Iterator iter = deviceEnergyModel.Begin (); iter != deviceEnergyModel.End (); iter ++) {
      double energyConsumed = (*iter)->GetTotalEnergyConsumption ();
      NS_LOG_UNCOND ("End of simulation (" << Simulator::Now ().GetSeconds ()
                    << "s) Total energy consumed by radio (Station) = " << energyConsumed << "J");
      std::cout << "Total energy consumed by radio (Station): " << energyConsumed << std::endl;
      //std::cout << "Battery lifetime: " << ((capacityJoules / energyConsumed) * simulationTime) / 86400 << std::endl;
      //energy = energyConsumed;
      break;
  }

  /*GtkConfigStore config;
  config.ConfigureAttributes();*/

  Simulator::Destroy ();
  return 0;

}
