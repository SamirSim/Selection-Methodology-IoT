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
#include "ns3/rv-battery-model-helper.h"
#include "ns3/rv-battery-model.h"
#include "ns3/energy-source-container.h"
#include "ns3/device-energy-model-container.h"
#include "ns3/applications-module.h"
#include <iomanip>

using namespace ns3;
using namespace std;

double totalBytes = 0; // To compute success rate

std::list<double> sizes; // List containing frame sizes

NS_LOG_COMPONENT_DEFINE ("wifi-vbr");

static void GenerateTraffic (Ptr<Socket> socket,
							uint32_t pktCount, Time pktInterval, int index ) {
	if (pktCount > 0) {
		list<double>::iterator it = sizes.begin();
		advance(it, index);
		int pktSize = *it;

		socket->Send (Create<Packet> (pktSize));
		
		totalBytes += pktSize;
		
		Simulator::Schedule (pktInterval, &GenerateTraffic,
							socket, pktCount-1, pktInterval, index + 1);
	}
	else {
		socket->Close ();
	}
}

int main (int argc, char *argv[]) {
  SeedManager::SetSeed (3);  // Changes seed from default of 1 to 3
  SeedManager::SetRun (2);  // Changes run number from default of 1 to 7
  double simulationTime = 20; // Seconds
  uint32_t nWifi = 1; // Number of stations
  uint32_t MCS = 9; // Number of stations
  uint32_t txPower = 9; // Number of stations
  std::string trafficDirection = "upstream";
  double fps = 30.0; // FPS is considered constant (30)
  std::string traceFile = "2Mbps.txt"; // Video trace file for stochastic traffic
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

  bool hiddenStations = 0;

  double distance = 1.0; // Meters between AP and stations
  bool agregation = false; // Allow or not the packet agregation
  int channelWidth = 80; // BW Channel Width in MHz
  int sgi = 0; // Indicates whether Short Guard Interval is enabled or not (SGI==1 <==> GI=400ns)
  //Avanced parameters
  std::string propDelay = "ConstantSpeedPropagationDelayModel";
  std::string propLoss = "LogDistancePropagationLossModel";
  int spatialStreams = 1;

  //Energy parameters
  double tx = 0.52;  // in W
  double rx = 0.16;  // in W
  double txFactor = 0.93; // in mJ
  double rxFactor = 0.93; // in mJ
  double voltage = 12; // in W
  double batteryCap = 500; // Battery capacity in mAh

  int numPackets = 1000000000; // Max number of packets to be sent (+inf)

  CommandLine cmd (__FILE__);
  cmd.AddValue ("distance", "Distance in meters between the station and the access point", distance);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("MCS", "MCS", MCS);
  cmd.AddValue ("txPower", "TxPower in dBm", txPower);
  cmd.AddValue ("channelWidth", "Channel Width in MHz", channelWidth);
  cmd.AddValue ("propDelay", "Delay Propagation Model", propDelay);
  cmd.AddValue ("propLoss", "Distance Propagation Model", propLoss);
  cmd.AddValue ("spatialStreams", "Number of Spatial Streams", spatialStreams);
  cmd.AddValue ("batteryCap", "Battery Capacity in mAh", batteryCap);
  cmd.AddValue ("voltage", "Battery voltage in Volts", voltage);
  cmd.AddValue ("txCurrent", "Tx current draw in mA", txCurrent);
  cmd.AddValue ("rxCurrent", "Rx current draw in mA", rxCurrent);
  cmd.AddValue ("idleCurrent", "Idle current draw in mA", idleCurrent);
  cmd.AddValue ("ccaBusyCurrent", "CCA Busy voltage in Volts", ccaBusyCurrent);
  cmd.AddValue ("nWifi", "Number of stations", nWifi);
  cmd.AddValue ("fps", "Frames per second", fps);
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

	Time::SetResolution (Time::NS);
  }

  double period = 1 / fps;

  LogComponentEnableAll (LOG_PREFIX_FUNC);
  LogComponentEnableAll (LOG_PREFIX_NODE);
  LogComponentEnableAll (LOG_PREFIX_TIME);

  CsvReader csv (traceFile);

  while (csv.FetchNextRow ()) {
	if (csv.IsBlankRow ()) {
		continue;
	}

	// Read the trace and get the arrival information
	double s;
	bool ok = csv.GetValue (3, s);
	sizes.push_back(s);
		
	if (!ok) {
		// Handle error, then
		continue;
	}
  }

  YansWifiChannelHelper channel;
  channel.AddPropagationLoss ("ns3::"+propLoss);
  channel.SetPropagationDelay("ns3::"+propDelay);

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
	std::cout << "here" << std::endl;
	// AP is between the two stations, each station being located at 5 meters from the AP.
	// The distance between the two stations is thus equal to 10 meters.
	// Since the wireless range is limited to 5 meters, the two stations are hidden from each other.
	
	mobility.SetPositionAllocator (positionAlloc);

	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

	mobility.Install (wifiApNode);
	mobility.Install (wifiStaNodes);
  }
  else {
	// Setting stations' positions
	for (uint32_t i = 0; i < nWifi; i++) {
		positionAlloc->Add (Vector (distance, 0.0, 0.0));
	}

	mobility.SetPositionAllocator (positionAlloc);
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (wifiStaNodes);

	Ptr<ListPositionAllocator> positionAllocAp = CreateObject<ListPositionAllocator> ();
	positionAllocAp->Add (Vector (0.0, 0.0, 0.0));
	mobility.SetPositionAllocator (positionAllocAp);
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (wifiApNode);

	if (latency) {
	  positionAllocAp->Add (Vector (distance, 0.0, 0.0));
	  mobility.SetPositionAllocator (positionAllocAp);
	  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	  mobility.Install (wifiProbingNode);
	}
  }
  YansWifiPhyHelper phy;
  phy.SetChannel (channel.Create ());

  WifiMacHelper mac;
  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211ac);

  std::ostringstream oss;
  oss << "VhtMcs" << MCS;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode", StringValue (oss.str ()),
								"ControlMode", StringValue (oss.str ()));

  Ssid ssid = Ssid ("ns3-80211ac");

  // Installing phy & mac layers on the overloading stations
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

  // Setting IP addresses
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

  if (latency) {
	// UDP Echo Server application to be installed in the AP
	int echoPort = 10;
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
	  echoClient1.SetAttribute("Interval", TimeValue(Seconds(2)));
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

  if (agregation == false) {
	// Disable A-MPDU & A-MSDU in AP
	Ptr<NetDevice> dev = wifiApNode.Get(0)-> GetDevice(0);
	Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice> (dev);
	wifi_dev->GetMac ()->SetAttribute ("BE_MaxAmpduSize", UintegerValue (0));
	wifi_dev->GetMac ()->SetAttribute ("BE_MaxAmsduSize", UintegerValue (0));
  }

  // Set txPower
  for (uint32_t index = 0; index < nWifi; ++index) {
	Ptr<WifiPhy> phy_tx = dynamic_cast<WifiNetDevice*>(GetPointer((staDevices.Get(index))))->GetPhy();
	phy_tx->SetTxPowerEnd(txPower);
	phy_tx->SetTxPowerStart(txPower);
  }

  Ptr<WifiPhy> phy_tx = dynamic_cast<WifiNetDevice*>(GetPointer((apDevice.Get(0))))->GetPhy();
  phy_tx->SetTxPowerEnd(txPower);
  phy_tx->SetTxPowerStart(txPower);

  if (latency) {
	Ptr<WifiPhy> phy_tx = dynamic_cast<WifiNetDevice*>(GetPointer((probingDevice.Get(0))))->GetPhy();
	phy_tx->SetTxPowerEnd(txPower);
	phy_tx->SetTxPowerStart(txPower);
  }

  /* Setting applications */
  ApplicationContainer sourceApplications, sinkApplications;
  uint32_t portNumber = 9;

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  InetSocketAddress recvSocket = InetSocketAddress ("10.0.0.1", portNumber);
  Address sinkLocalAddress(recvSocket);
  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", sinkLocalAddress);
  ApplicationContainer sinkApp = sinkHelper.Install (wifiApNode.Get(0));
  sinkApplications.Add (sinkApp);
  sinkApp.Start(Seconds(0.0));
  sinkApp.Stop(Seconds(simulationTime+1));

  double min = 0.0;
  double max = 0.5;

  for (uint32_t index = 0; index < nWifi; ++index) {
	if (agregation == false) {
		// Disable A-MPDU & A-MSDU in each station
		Ptr<NetDevice> dev = wifiStaNodes.Get (index)->GetDevice (0);
		Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice> (dev);
		wifi_dev->GetMac ()->SetAttribute ("BE_MaxAmpduSize", UintegerValue (0));
		wifi_dev->GetMac ()->SetAttribute ("BE_MaxAmsduSize", UintegerValue (0));
	}

	Ptr<Socket> source = Socket::CreateSocket (wifiStaNodes.Get (index), tid);
	//InetSocketAddress remote =  recvSocket;
	//source->SetAllowBroadcast (true);
	source->Connect (recvSocket);
	
	Time interPacketInterval = Seconds (period);
	
	Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
	x->SetAttribute ("Min", DoubleValue (min));
	x->SetAttribute ("Max", DoubleValue (max));

	double value = 1 + x->GetValue ();

	Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
								  Seconds (value) , &GenerateTraffic,
								  source, numPackets, interPacketInterval, 0);
  }

  DeviceEnergyModelContainer deviceModels;

  double capacityJoules = (batteryCap / 1000.0) * voltage * 3600; // 5.2 Ah * 12 V * 3600

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
                      DoubleValue(voltage));
    rvModelHelper.Set ("RvBatteryModelCutoffVoltage", 
                      DoubleValue(0)); 
    rvModelHelper.Set ("RvBatteryModelAlphaValue", 
                      DoubleValue(capacityJoules / voltage));
    EnergySourceContainer sources = rvModelHelper.Install(wifiStaNodes);
    WifiRadioEnergyModelHelper radioEnergyHelper;
    
    deviceModels = radioEnergyHelper.Install (staDevices, sources);
  }

  AsciiTraceHelper ascii;

  phy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
  //std::string s = "vbr/"+std::to_string(nWifi)+"-"+dataRate+"-"+std::to_string(MCS)+"-"+std::to_string(payloadSize);
  //phy.EnableAsciiAll (ascii.CreateFileStream(s+".tr"));
  //phy.EnablePcap (s+".pcap", apDevice.Get(0), false, true);

  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();

  std::cout << std::fixed;
  std::cout << std::setprecision(2);
  
  if (energyRatio || energyPower) {
	double energy = 0;
	for (DeviceEnergyModelContainer::Iterator iter = deviceModels.Begin (); iter != deviceModels.End (); iter ++) {
	  double energyConsumed = (*iter)->GetTotalEnergyConsumption ();
	  NS_LOG_UNCOND ("End of simulation (" << Simulator::Now ().GetSeconds ()
					<< "s) Total energy consumed by radio (Station) = " << energyConsumed << "J");
	  std::cout << "Total energy consumed by radio (Station): " << energyConsumed << std::endl;
	  std::cout << "Battery lifetime: " << ((capacityJoules / energyConsumed) * simulationTime) / 86400 << std::endl;
	  energy = energyConsumed;
	  break;
	}
  }

  double totalPacketsThrough = 0, throughput = 0;
  if (trafficDirection == "upstream") {
	totalPacketsThrough = DynamicCast<PacketSink> (sinkApplications.Get (0))->GetTotalRx (); // Number of Bytes
	throughput += ((totalPacketsThrough * 8) / ((simulationTime) * 1024 * 1024)); //Mbit/s
	std::cout << "Throughput: " << throughput << std::endl;
	if (successRate) std::cout << "Success rate: " << (totalPacketsThrough/totalBytes) * 100 << std::endl;
	//std::cout <<  "Total received: " << totalPacketsThrough << " Total sent: " << totalBytes/nWifi << std::endl
  }
  else {
	for (uint32_t index = 0; index < sinkApplications.GetN (); ++index) {
	  totalPacketsThrough += DynamicCast<PacketSink> (sinkApplications.Get (index))->GetTotalRx ();
	}
  }

  double totalSentPackets = ((1/period) * simulationTime ) * nWifi; // Estimation of number of generated packets in the network
  //double totalReceivedPackets = totalPacketsThrough / payloadSize; // Number of total received packets

  if (energyRatio) {
	double receivedByteStation = totalPacketsThrough / nWifi; // Number of successfuly received bytes from one station
	std::cout << "Number of received bytes from one station: " << receivedByteStation << std::endl;
  }

  if (crossFactor) {
	std::cout << "Number of generated packets by one station: " << totalSentPackets / nWifi << std::endl; // Number of generated packets by station
  }
  
  Simulator::Destroy ();

  return 0;
}

/*/
auto ipv4 = wifiApNode.Get (0)->GetObject<Ipv4> ();
const auto address = ipv4->GetAddress (1, 0).GetLocal ();
InetSocketAddress sinkSocket (address, portNumber);
sinkSocket->SetRecvCallback (MakeCallback (&ReceivePacket))
Ptr<Socket> recvSink = Socket::CreateSocket (wifiApNode.Get (0), tid);
InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
recvSink->Bind (local);
recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));
/*/