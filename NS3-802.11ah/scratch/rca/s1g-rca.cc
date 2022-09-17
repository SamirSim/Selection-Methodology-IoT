/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 MIRKO BANCHI
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
 */

#include "s1g-rca.h"
#include <fstream>
#include "ns3/string.h"
#include "ns3/core-module.h"

NS_LOG_COMPONENT_DEFINE("s1g-wifi-network-tim-raw");

uint32_t AssocNum = 0;
int64_t AssocTime = 0;
uint32_t StaNum = 0;
NetDeviceContainer staDeviceCont;
const int MaxSta = 8000;

Configuration config;
Statistics stats;
SimulationEventManager eventManager;

class assoc_record {
public:
	assoc_record();
	bool GetAssoc();
	void SetAssoc(std::string context, Mac48Address address);
	void UnsetAssoc(std::string context, Mac48Address address);
	void setstaid(uint16_t id);
private:
	bool assoc;
	uint16_t staid;
};

assoc_record::assoc_record() {
	assoc = false;
	staid = 65535;
}

void assoc_record::setstaid(uint16_t id) {
	staid = id;
}

void assoc_record::SetAssoc(std::string context, Mac48Address address) {
	assoc = true;
}

void assoc_record::UnsetAssoc(std::string context, Mac48Address address) {
	assoc = false;
}

bool assoc_record::GetAssoc() {
	return assoc;
}

typedef std::vector<assoc_record *> assoc_recordVector;
assoc_recordVector assoc_vector;

uint32_t GetAssocNum() {
	AssocNum = 0;
	for (assoc_recordVector::const_iterator index = assoc_vector.begin();
			index != assoc_vector.end(); index++) {
		if ((*index)->GetAssoc()) {
			AssocNum++;
		}
	}
	return AssocNum;
}

void PrintPositions (NodeContainer wifiStaNode)
{
  
    Ptr<RandomWalk2dMobilityModel> mob = wifiStaNode.Get(1)->GetObject<RandomWalk2dMobilityModel>();
    Vector pos = mob->GetPosition ();
    std::cout << "POS: x=" << pos.x << ", y=" << pos.y << ", z=" << pos.z << "," << Simulator::Now ().GetSeconds ()<< std::endl;
    //mob->SetVelocity (Vector(1,0,0));
    
    Simulator::Schedule(Seconds(1), &PrintPositions, wifiStaNode);
}

void PopulateArpCache() {
	Ptr<ArpCache> arp = CreateObject<ArpCache>();
	arp->SetAliveTimeout(Seconds(3600 * 24 * 365));
	for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i) {
		Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol>();
		NS_ASSERT(ip != 0);
		ObjectVectorValue interfaces;
		ip->GetAttribute("InterfaceList", interfaces);
		for (ObjectVectorValue::Iterator j = interfaces.Begin();
				j != interfaces.End(); j++) {
			Ptr<Ipv4Interface> ipIface =
					(j->second)->GetObject<Ipv4Interface>();
			NS_ASSERT(ipIface != 0);
			Ptr<NetDevice> device = ipIface->GetDevice();
			NS_ASSERT(device != 0);
			Mac48Address addr = Mac48Address::ConvertFrom(device->GetAddress());
			for (uint32_t k = 0; k < ipIface->GetNAddresses(); k++) {
				Ipv4Address ipAddr = ipIface->GetAddress(k).GetLocal();
				if (ipAddr == Ipv4Address::GetLoopback())
					continue;
				ArpCache::Entry * entry = arp->Add(ipAddr);
				entry->MarkWaitReply(0);
				entry->MarkAlive(addr);
				std::cout << "Arp Cache: Adding the pair (" << addr << ","
						<< ipAddr << ")" << std::endl;
			}
		}
	}
	for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i) {
		Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol>();
		NS_ASSERT(ip != 0);
		ObjectVectorValue interfaces;
		ip->GetAttribute("InterfaceList", interfaces);
		for (ObjectVectorValue::Iterator j = interfaces.Begin();
				j != interfaces.End(); j++) {
			Ptr<Ipv4Interface> ipIface =
					(j->second)->GetObject<Ipv4Interface>();
			ipIface->SetAttribute("ArpCache", PointerValue(arp));
		}
	}
}

uint16_t ngroup;
uint16_t nslot;
RPSVector configureRAW(RPSVector rpslist, string RAWConfigFile) {
	uint16_t NRPS = 0;
	uint16_t NRAWPERBEACON = 0;
	uint16_t Value = 0;
	uint32_t page = 0;
	uint32_t aid_start = 0;
	uint32_t aid_end = 0;
	uint32_t rawinfo = 0;

	ifstream myfile(RAWConfigFile);
	//1. get info from config file

	//2. define RPS
	if (myfile.is_open()) {
		myfile >> NRPS;
		int totalNumSta = 0;
		for (uint16_t kk = 0; kk < NRPS; kk++) // number of beacons covering all raw groups
		{
			RPS *m_rps = new RPS;
			myfile >> NRAWPERBEACON;
			ngroup = NRAWPERBEACON;
			for (uint16_t i = 0; i < NRAWPERBEACON; i++) // raw groups in one beacon
			{
				//RPS *m_rps = new RPS;
				RPS::RawAssignment *m_raw = new RPS::RawAssignment;

				myfile >> Value;
				m_raw->SetRawControl(Value);  //support paged STA or not
				myfile >> Value;
				m_raw->SetSlotCrossBoundary(Value);
				myfile >> Value;
				m_raw->SetSlotFormat(Value);
				myfile >> Value;
				m_raw->SetSlotDurationCount(Value);
				myfile >> Value;
				nslot = Value;
				m_raw->SetSlotNum(Value);
				myfile >> page;
				myfile >> aid_start;
				myfile >> aid_end;
				rawinfo = (aid_end << 13) | (aid_start << 2) | page;
				m_raw->SetRawGroup(rawinfo);
				totalNumSta += aid_end - aid_start + 1;
				m_rps->SetRawAssignment(*m_raw);
				delete m_raw;
			}
			rpslist.rpsset.push_back(m_rps);
			//config.nRawGroupsPerRpsList.push_back(NRAWPERBEACON);
		}
		myfile.close();
		config.NRawSta = totalNumSta;

				/*rpslist.rpsset[rpslist.rpsset.size() - 1]->GetRawAssigmentObj(
						NRAWPERBEACON - 1).GetRawGroupAIDEnd();*/
	} else
		cout << "Unable to open RAW configuration file \n";

	return rpslist;
}

/*
pageslice element and TIM(DTIM) together accomplish page slicing.

Prior knowledge:
802.11ah support up to 8192 stations, they are constructed into: page, block,
 subblock, sta.
there are 13 bit represent the AID of stations.
 AID[11-12] represent page.
 AID[6-10] represent block.
 AID[3-5] represent subblock.
 AID[0-2] represent sta.

A TIM(DTIM) element only support one page
A Page slice element only support one page

 Concept of page slicing:
 Between two DTIM beacon, there are many TIM beacons, only allow a TIM beacon include some blocks of one page is called page slice. One TIM beacon is called a page slice.
 Page slcie element specify number of page slice between two DTIM, number of blocks in each
 page slice.
 Page slice element only appears together with DTIM.

 Details:
 Page slice element also indicates AP has buffered data for which block, if a station is in that block, the station should first sleep, then wake up at coresponding page slice(TIM beacon) which includes that block.

 When station wake up at that block, it check whether AP has data for itself. If has, keep awake to receive packets and go to sleep in the next beacon.
 */

void configurePageSlice (void)
{
    config.pageS.SetPageindex (config.pageIndex);
    config.pageS.SetPagePeriod (config.pagePeriod); //2 TIM groups between DTIMs
    config.pageS.SetPageSliceLen (config.pageSliceLength); //each TIM group has 1 block (2 blocks in 2 TIM groups)
    config.pageS.SetPageSliceCount (config.pageSliceCount);
    config.pageS.SetBlockOffset (config.blockOffset);
    config.pageS.SetTIMOffset (config.timOffset);
    //std::cout << "pageIndex=" << (int)config.pageIndex << ", pagePeriod=" << (int)config.pagePeriod << ", pageSliceLength=" << (int)config.pageSliceLength << ", pageSliceCount=" << (int)config.pageSliceCount << ", blockOffset=" << (int)config.blockOffset << ", timOffset=" << (int)config.timOffset << std::endl;
    // page 0
    // 8 TIM(page slice) for one page
    // 4 block (each page)
    // 8 page slice
    // both offset are 0
}

void configureTIM (void)
{
    config.tim.SetPageIndex (config.pageIndex);
    if (config.pageSliceCount)
    	config.tim.SetDTIMPeriod (config.pageSliceCount); // not necessarily the same
    else
    	config.tim.SetDTIMPeriod (1);

    //std::cout << "DTIM period=" << (int)config.pagePeriod << std::endl;
}

void checkRawAndTimConfiguration (void)
{
	std::cout << "Checking RAW and TIM configuration..." << std::endl;
	bool configIsCorrect = true;
	NS_ASSERT (config.rps.rpsset.size());
	// Number of TIM groups in a single page has to equal number of different RPS elements because
	// If #TIM > #RPS, the same RPS will be used in more than 1 TIM and that is wrong because
	// each TIM can accommodate different AIDs (same RPS means same stations in RAWs)
	/*
	if(config.pageSliceCount)
    {
	NS_ASSERT (config.pageSliceCount == config.rps.rpsset.size());
    }
	*/
	for (uint32_t j = 0; j < config.rps.rpsset.size(); j++)
	{
		uint32_t totalRawTime = 0;
		for (uint32_t i = 0; i < config.rps.rpsset[j]->GetNumberOfRawGroups(); i++)
		{
			totalRawTime += (120 * config.rps.rpsset[j]->GetRawAssigmentObj(i).GetSlotDurationCount() + 500) * config.rps.rpsset[j]->GetRawAssigmentObj(i).GetSlotNum();
			auto aidStart = config.rps.rpsset[j]->GetRawAssigmentObj(i).GetRawGroupAIDStart();
			auto aidEnd = config.rps.rpsset[j]->GetRawAssigmentObj(i).GetRawGroupAIDEnd();
			std::cout << aidStart << " " << aidEnd << " " << j << std::endl;
			configIsCorrect = check (aidStart, j) && check (aidEnd, j);
			// AIDs in each RPS must comply with TIM in the following way:
			// TIM0: 1-63; TIM1: 64-127; TIM2: 128-191; ...; TIM32: 1983-2047
			// If RPS that belongs to TIM0 includes other AIDs (other than range [1-63]) configuration is incorrect

			NS_ASSERT (configIsCorrect);
		}
		NS_ASSERT (totalRawTime <= config.BeaconInterval);
	}
}

bool check (uint16_t aid, uint32_t index)
{
	uint8_t block = (aid >> 6 ) & 0x001f;
	NS_ASSERT (config.pageS.GetPageSliceLen() > 0);
	uint8_t toTim = (block - config.pageS.GetBlockOffset()) % config.pageS.GetPageSliceLen();
	std::cout << "toTim " << (block - config.pageS.GetBlockOffset()) % config.pageS.GetPageSliceLen() << std::endl;
	return toTim == index;
}


void sendStatistics(bool schedule) {
	eventManager.onUpdateStatistics(stats);
	eventManager.onUpdateSlotStatistics(
			transmissionsPerTIMGroupAndSlotFromAPSinceLastInterval,
			transmissionsPerTIMGroupAndSlotFromSTASinceLastInterval);
	// reset
	std::fill(transmissionsPerTIMGroupAndSlotFromAPSinceLastInterval.begin(),
			transmissionsPerTIMGroupAndSlotFromAPSinceLastInterval.end(), 0);
	std::fill(transmissionsPerTIMGroupAndSlotFromSTASinceLastInterval.begin(),
			transmissionsPerTIMGroupAndSlotFromSTASinceLastInterval.end(), 0);

	if (schedule)
		Simulator::Schedule(Seconds(config.visualizerSamplingInterval),	&sendStatistics, true);
}

void onSTADeassociated(int i) {
	eventManager.onNodeDeassociated(*nodes[i]);
}

void updateNodesQueueLength() {
	for (uint32_t i = 0; i < config.Nsta; i++) {
		nodes[i]->UpdateQueueLength();
		stats.get(i).EDCAQueueLength = nodes[i]->queueLength;
	}
	Simulator::Schedule(Seconds(0.5), &updateNodesQueueLength);
}

void onSTAAssociated(int i) {
	cout << "Node " << std::to_string(i) << " is associated and has aid "
			<< nodes[i]->aId << endl;

	for (int k = 0; k < config.rps.rpsset.size(); k++) {
		for (int j = 0; j < config.rps.rpsset[k]->GetNumberOfRawGroups(); j++) {
			if (config.rps.rpsset[k]->GetRawAssigmentObj(j).GetRawGroupAIDStart()
					<= i + 1
					&& i + 1
							<= config.rps.rpsset[k]->GetRawAssigmentObj(j).GetRawGroupAIDEnd()) {
				nodes[i]->rpsIndex = k + 1;
				nodes[i]->rawGroupNumber = j + 1;
				nodes[i]->rawSlotIndex =
						nodes[i]->aId
								% config.rps.rpsset[k]->GetRawAssigmentObj(j).GetSlotNum()
								+ 1;
				/*cout << "Node " << i << " with AID " << (int)nodes[i]->aId << " belongs to " << (int)nodes[i]->rawSlotIndex << " slot of RAW group "
				 << (int)nodes[i]->rawGroupNumber << " within the " << (int)nodes[i]->rpsIndex << " RPS." << endl;
				 */
			}
		}
	}

	eventManager.onNodeAssociated(*nodes[i]);

	// RPS, Raw group and RAW slot assignment

	if (GetAssocNum() == config.Nsta) {
		cout << "All " << AssocNum << " stations associated at " << Simulator::Now ().GetMicroSeconds () <<", configuring clients & server" << endl;

		// association complete, start sending packets
		stats.TimeWhenEverySTAIsAssociated = Simulator::Now();

		if (config.trafficType == "udpcamera") {
			configureUDPEchoServer();
			configureCameraClients();
		} else if (config.trafficType == "udpecho") {
			configureUDPEchoServer();
			configureUDPEchoClients();
		} else if (config.trafficType == "tcpecho") {
			configureTCPEchoServer();
			configureTCPEchoClients();
		} else if (config.trafficType == "tcppingpong") {
			configureTCPPingPongServer();
			configureTCPPingPongClients();
		} else if (config.trafficType == "tcpipcamera") {
			configureTCPIPCameraServer();
			configureTCPIPCameraClients();
		} else if (config.trafficType == "tcpfirmware") {
			configureTCPFirmwareServer();
			configureTCPFirmwareClients();
		} else if (config.trafficType == "tcpsensor") {
			configureTCPSensorServer();
			configureTCPSensorClients();
		}
		updateNodesQueueLength();
	}
}

void RpsIndexTrace(uint16_t oldValue, uint16_t newValue) {
	currentRps = newValue;
	//cout << "RPS: " << newValue << " at " << Simulator::Now().GetMicroSeconds() << endl;
}

void RawGroupTrace(uint8_t oldValue, uint8_t newValue) {
	currentRawGroup = newValue;
	//cout << "	group " << std::to_string(newValue) << " at " << Simulator::Now().GetMicroSeconds() << endl;
}

void RawSlotTrace(uint8_t oldValue, uint8_t newValue) {
	currentRawSlot = newValue;
	//cout << "		slot " << std::to_string(newValue) << " at " << Simulator::Now().GetMicroSeconds() << endl;
}

void configureNodes(NodeContainer& wifiStaNode, NetDeviceContainer& staDevice) {
	cout << "Configuring STA Node trace sources..." << endl;

	for (uint32_t i = 0; i < config.Nsta; i++) {
		//cout << "Hooking up trace sources for STA " << i << endl;

		NodeEntry* n = new NodeEntry(i, &stats, wifiStaNode.Get(i),
				staDevice.Get(i));

		n->SetAssociatedCallback([ = ] {onSTAAssociated(i);});
		n->SetDeassociatedCallback([ = ] {onSTADeassociated(i);});

		nodes.push_back(n);
		// hook up Associated and Deassociated events
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/Assoc",
				MakeCallback(&NodeEntry::SetAssociation, n));
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/DeAssoc",
				MakeCallback(&NodeEntry::UnsetAssociation, n));
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/NrOfTransmissionsDuringRAWSlot",
				MakeCallback(
						&NodeEntry::OnNrOfTransmissionsDuringRAWSlotChanged,
						n));	//not implem

		//Config::Connect("/NodeList/" + std::to_string(i) + "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/S1gBeaconMissed", MakeCallback(&NodeEntry::OnS1gBeaconMissed, n));

		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/PacketDropped",
				MakeCallback(&NodeEntry::OnMacPacketDropped, n));
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/Collision",
				MakeCallback(&NodeEntry::OnCollision, n));
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/TransmissionWillCrossRAWBoundary",
				MakeCallback(&NodeEntry::OnTransmissionWillCrossRAWBoundary,
						n)); //?

		// hook up TX
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyTxBegin",
				MakeCallback(&NodeEntry::OnPhyTxBegin, n));
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyTxEnd",
				MakeCallback(&NodeEntry::OnPhyTxEnd, n));
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyTxDropWithReason",
				MakeCallback(&NodeEntry::OnPhyTxDrop, n)); //?

		// hook up RX
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyRxBegin",
				MakeCallback(&NodeEntry::OnPhyRxBegin, n));
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyRxEnd",
				MakeCallback(&NodeEntry::OnPhyRxEnd, n));
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyRxDropWithReason",
				MakeCallback(&NodeEntry::OnPhyRxDrop, n));

		// hook up MAC traces
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/RemoteStationManager/MacTxRtsFailed",
				MakeCallback(&NodeEntry::OnMacTxRtsFailed, n)); //?
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/RemoteStationManager/MacTxDataFailed",
				MakeCallback(&NodeEntry::OnMacTxDataFailed, n));
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/RemoteStationManager/MacTxFinalRtsFailed",
				MakeCallback(&NodeEntry::OnMacTxFinalRtsFailed, n)); //?
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/RemoteStationManager/MacTxFinalDataFailed",
				MakeCallback(&NodeEntry::OnMacTxFinalDataFailed, n)); //?

		// hook up PHY State change
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/Phy/State/State",
				MakeCallback(&NodeEntry::OnPhyStateChange, n));

	}
}

int getBandwidth(string dataMode) {
	if (dataMode == "MCS1_0" || dataMode == "MCS1_1" || dataMode == "MCS1_2"
			|| dataMode == "MCS1_3" || dataMode == "MCS1_4"
			|| dataMode == "MCS1_5" || dataMode == "MCS1_6"
			|| dataMode == "MCS1_7" || dataMode == "MCS1_8"
			|| dataMode == "MCS1_9" || dataMode == "MCS1_10")
		return 1;

	else if (dataMode == "MCS2_0" || dataMode == "MCS2_1"
			|| dataMode == "MCS2_2" || dataMode == "MCS2_3"
			|| dataMode == "MCS2_4" || dataMode == "MCS2_5"
			|| dataMode == "MCS2_6" || dataMode == "MCS2_7"
			|| dataMode == "MCS2_8" || dataMode == "MCS2_10")
		return 2;

	else if (dataMode == "MCS4_0" || dataMode == "MCS4_1"
			|| dataMode == "MCS4_2" || dataMode == "MCS4_3"
			|| dataMode == "MCS4_4" || dataMode == "MCS4_5"
			|| dataMode == "MCS4_6" || dataMode == "MCS4_7"
			|| dataMode == "MCS4_8" || dataMode == "MCS4_10")
		return 4;

	return 0;
}

string getWifiMode(string dataMode) {
	if (dataMode == "MCS1_0")
		return "OfdmRate300KbpsBW1MHz";
	else if (dataMode == "MCS1_1")
		return "OfdmRate600KbpsBW1MHz";
	else if (dataMode == "MCS1_2")
		return "OfdmRate900KbpsBW1MHz";
	else if (dataMode == "MCS1_3")
		return "OfdmRate1_2MbpsBW1MHz";
	else if (dataMode == "MCS1_4")
		return "OfdmRate1_8MbpsBW1MHz";
	else if (dataMode == "MCS1_5")
		return "OfdmRate2_4MbpsBW1MHz";
	else if (dataMode == "MCS1_6")
		return "OfdmRate2_7MbpsBW1MHz";
	else if (dataMode == "MCS1_7")
		return "OfdmRate3MbpsBW1MHz";
	else if (dataMode == "MCS1_8")
		return "OfdmRate3_6MbpsBW1MHz";
	else if (dataMode == "MCS1_9")
		return "OfdmRate4MbpsBW1MHz";
	else if (dataMode == "MCS1_10")
		return "OfdmRate150KbpsBW1MHz";

	else if (dataMode == "MCS2_0")
		return "OfdmRate650KbpsBW2MHz";
	else if (dataMode == "MCS2_1")
		return "OfdmRate1_3MbpsBW2MHz";
	else if (dataMode == "MCS2_2")
		return "OfdmRate1_95MbpsBW2MHz";
	else if (dataMode == "MCS2_3")
		return "OfdmRate2_6MbpsBW2MHz";
	else if (dataMode == "MCS2_4")
		return "OfdmRate3_9MbpsBW2MHz";
	else if (dataMode == "MCS2_5")
		return "OfdmRate5_2MbpsBW2MHz";
	else if (dataMode == "MCS2_6")
		return "OfdmRate5_85MbpsBW2MHz";
	else if (dataMode == "MCS2_7")
		return "OfdmRate6_5MbpsBW2MHz";
	else if (dataMode == "MCS2_8")
		return "OfdmRate7_8MbpsBW2MHz";
	
	else if (dataMode == "MCS4_0")
		return "OfdmRate1_5MbpsBW4MHz";
	else if (dataMode == "MCS4_1")
		return "OfdmRate1_35MbpsBW4MHz";
	else if (dataMode == "MCS4_2")
		return "OfdmRate2_7MbpsBW4MHz";
	else if (dataMode == "MCS4_3")
		return "OfdmRate3MbpsBW4MHz";
	else if (dataMode == "MCS4_4")
		return "OfdmRate4_05MbpsBW4MHz";
	else if (dataMode == "MCS4_5")
		return "OfdmRate4_5MbpsBW4MHz";
	else if (dataMode == "MCS4_6")
		return "OfdmRate5_4MbpsBW4MHz";
	else if (dataMode == "MCS4_7")
		return "OfdmRate6MbpsBW4MHz";
	else if (dataMode == "MCS4_8")
		return "OfdmRate8_1MbpsBW4MHz";
	return "";
}

void OnAPPhyRxDrop(std::string context, Ptr<const Packet> packet,
		DropReason reason) {
	// THIS REQUIRES PACKET METADATA ENABLE!
	auto pCopy = packet->Copy();
	auto it = pCopy->BeginItem();
	while (it.HasNext()) {

		auto item = it.Next();
		Callback<ObjectBase *> constructor = item.tid.GetConstructor();

		ObjectBase *instance = constructor();
		Chunk *chunk = dynamic_cast<Chunk *>(instance);
		chunk->Deserialize(item.current);

		if (dynamic_cast<WifiMacHeader*>(chunk)) {
			WifiMacHeader* hdr = (WifiMacHeader*) chunk;

			int staId = -1;
			if (!config.useV6) {
				for (uint32_t i = 0; i < staNodeInterface.GetN(); i++) {
					if (wifiStaNode.Get(i)->GetDevice(0)->GetAddress()
							== hdr->GetAddr2()) {
						staId = i;
						break;
					}
				}
			} else {
				for (uint32_t i = 0; i < staNodeInterface6.GetN(); i++) {
					if (wifiStaNode.Get(i)->GetDevice(0)->GetAddress()
							== hdr->GetAddr2()) {
						staId = i;
						break;
					}
				}
			}
			if (staId != -1) {
				stats.get(staId).NumberOfDropsByReasonAtAP[reason]++;
			}
			delete chunk;
			break;
		} else
			delete chunk;
	}

}

void OnAPPacketToTransmitReceived(string context, Ptr<const Packet> packet,
		Mac48Address to, bool isScheduled, bool isDuringSlotOfSTA,
		Time timeLeftInSlot) {
	int staId = -1;
	if (!config.useV6) {
		for (uint32_t i = 0; i < staNodeInterface.GetN(); i++) {
			if (wifiStaNode.Get(i)->GetDevice(0)->GetAddress() == to) {
				staId = i;
				break;
			}
		}
	} else {
		for (uint32_t i = 0; i < staNodeInterface6.GetN(); i++) {
			if (wifiStaNode.Get(i)->GetDevice(0)->GetAddress() == to) {
				staId = i;
				break;
			}
		}
	}
	if (staId != -1) {
		if (isScheduled)
			stats.get(staId).NumberOfAPScheduledPacketForNodeInNextSlot++;
		else {
			stats.get(staId).NumberOfAPSentPacketForNodeImmediately++;
			stats.get(staId).APTotalTimeRemainingWhenSendingPacketInSameSlot +=
					timeLeftInSlot;
		}
	}
}

void onChannelTransmission(Ptr<NetDevice> senderDevice, Ptr<Packet> packet) {
	int rpsIndex = currentRps - 1;
	int rawGroup = currentRawGroup - 1;
	int slotIndex = currentRawSlot - 1;
	//cout << rpsIndex << "		" << rawGroup << "		" << slotIndex << "		" << endl;

	uint64_t iSlot = slotIndex;
	if (rpsIndex > 0)
		for (int r = rpsIndex - 1; r >= 0; r--)
			for (int g = 0; g < config.rps.rpsset[r]->GetNumberOfRawGroups(); g++)
				iSlot += config.rps.rpsset[r]->GetRawAssigmentObj(g).GetSlotNum();

	if (rawGroup > 0)
		for (int i = rawGroup - 1; i >= 0; i--)
			iSlot += config.rps.rpsset[rpsIndex]->GetRawAssigmentObj(i).GetSlotNum();

	if (rpsIndex >= 0 && rawGroup >= 0 && slotIndex >= 0)
	{
		if (senderDevice->GetAddress() == apDevice.Get(0)->GetAddress())
		{
			// from AP
			transmissionsPerTIMGroupAndSlotFromAPSinceLastInterval[iSlot] += packet->GetSerializedSize();
		}
		else
		{
			// from STA
			transmissionsPerTIMGroupAndSlotFromSTASinceLastInterval[iSlot] += packet->GetSerializedSize();

		}
	}
	//std::cout << "------------- packetSerializedSize = " << packet->GetSerializedSize() << std::endl;
	//std::cout << "------------- txAP[" << iSlot <<"] = " << transmissionsPerTIMGroupAndSlotFromAPSinceLastInterval[iSlot] << std::endl;
	//std::cout << "------------- txSTA[" << iSlot <<"] = " << transmissionsPerTIMGroupAndSlotFromSTASinceLastInterval[iSlot] << std::endl;

}

int getSTAIdFromAddress(Ipv4Address from) {
	int staId = -1;
	for (int i = 0; i < staNodeInterface.GetN(); i++) {
		if (staNodeInterface.GetAddress(i) == from) {
			staId = i;
			break;
		}
	}
	return staId;
}

void udpPacketReceivedAtServer(Ptr<const Packet> packet, Address from) { //works
	//cout << "+++++++++++udpPacketReceivedAtServer" << endl;
	int staId = getSTAIdFromAddress(
			InetSocketAddress::ConvertFrom(from).GetIpv4());
	if (staId != -1)
		nodes[staId]->OnUdpPacketReceivedAtAP(packet);
	else
		cout << "*** Node could not be determined from received packet at AP "
				<< endl;
}

void tcpPacketReceivedAtServer(Ptr<const Packet> packet, Address from) {
	int staId = getSTAIdFromAddress(
			InetSocketAddress::ConvertFrom(from).GetIpv4());
	if (staId != -1)
		nodes[staId]->OnTcpPacketReceivedAtAP(packet);
	else
		cout << "*** Node could not be determined from received packet at AP "
				<< endl;
}

void tcpRetransmissionAtServer(Address to) {
	int staId = getSTAIdFromAddress(Ipv4Address::ConvertFrom(to));
	if (staId != -1)
		nodes[staId]->OnTcpRetransmissionAtAP();
	else
		cout << "*** Node could not be determined from received packet at AP "
				<< endl;
}

void tcpPacketDroppedAtServer(Address to, Ptr<Packet> packet,
		DropReason reason) {
	int staId = getSTAIdFromAddress(Ipv4Address::ConvertFrom(to));
	if (staId != -1) {
		stats.get(staId).NumberOfDropsByReasonAtAP[reason]++;
	}
}

void tcpStateChangeAtServer(TcpSocket::TcpStates_t oldState,
		TcpSocket::TcpStates_t newState, Address to) {

	int staId = getSTAIdFromAddress(
			InetSocketAddress::ConvertFrom(to).GetIpv4());
	if (staId != -1)
		nodes[staId]->OnTcpStateChangedAtAP(oldState, newState);
	else
		cout << "*** Node could not be determined from received packet at AP "
				<< endl;

	//cout << Simulator::Now().GetMicroSeconds() << " ********** TCP SERVER SOCKET STATE CHANGED FROM " << oldState << " TO " << newState << endl;
}

void tcpIPCameraDataReceivedAtServer(Address from, uint16_t nrOfBytes) {
	int staId = getSTAIdFromAddress(
			InetSocketAddress::ConvertFrom(from).GetIpv4());
	if (staId != -1)
		nodes[staId]->OnTcpIPCameraDataReceivedAtAP(nrOfBytes);
	else
		cout << "*** Node could not be determined from received packet at AP "
				<< endl;
}

void configureUDPServer() {
	UdpServerHelper myServer(9);
	serverApp = myServer.Install(wifiApNode);
	serverApp.Get(0)->TraceConnectWithoutContext("Rx",
			MakeCallback(&udpPacketReceivedAtServer));
	serverApp.Start(Seconds(0));

}

ApplicationContainer sinkApplications, clientApp, anthonyApp;

void configureUDPEchoServer() {
	auto ipv4 = wifiApNode.Get (0)->GetObject<Ipv4> ();
    const auto address = ipv4->GetAddress (1, 0).GetLocal ();
    InetSocketAddress sinkSocket (address, 9);
    PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", sinkSocket);
	sinkApplications.Add (packetSinkHelper.Install (wifiApNode.Get (0)));
	sinkApplications.Start (Seconds (0.0));

	/*
	UdpEchoServerHelper myServer(9);
	serverApp = myServer.Install(wifiApNode);
	serverApp.Get(0)->TraceConnectWithoutContext("Rx",
			MakeCallback(&udpPacketReceivedAtServer));
	serverApp.Start(Seconds(0));
	*/
	
}

void configureTCPEchoServer() {
	TcpEchoServerHelper myServer(80);
	serverApp = myServer.Install(wifiApNode);
	wireTCPServer(serverApp);
	serverApp.Start(Seconds(0));
}

void configureTCPPingPongServer() {
	// TCP ping pong is a test for the new base tcp-client and tcp-server applications
	ObjectFactory factory;
	factory.SetTypeId(TCPPingPongServer::GetTypeId());
	factory.Set("Port", UintegerValue(81));

	Ptr<Application> tcpServer = factory.Create<TCPPingPongServer>();
	wifiApNode.Get(0)->AddApplication(tcpServer);

	auto serverApp = ApplicationContainer(tcpServer);
	wireTCPServer(serverApp);
	serverApp.Start(Seconds(0));
}

void configureTCPPingPongClients() {

	ObjectFactory factory;
	factory.SetTypeId(TCPPingPongClient::GetTypeId());
	factory.Set("Interval", TimeValue(MilliSeconds(config.trafficInterval)));
	factory.Set("PacketSize", UintegerValue(config.payloadSize));

	factory.Set("RemoteAddress",
			Ipv4AddressValue(apNodeInterface.GetAddress(0)));
	factory.Set("RemotePort", UintegerValue(81));

	Ptr<UniformRandomVariable> m_rv = CreateObject<UniformRandomVariable>();
	//std::cout << "Nsta = " << config.Nsta << std::endl;
	for (uint16_t i = 0; i < config.Nsta; i++) {

		Ptr<Application> tcpClient = factory.Create<TCPPingPongClient>();
		wifiStaNode.Get(i)->AddApplication(tcpClient);
		auto clientApp = ApplicationContainer(tcpClient);
		wireTCPClient(clientApp, i);

		double random = m_rv->GetValue(0, config.trafficInterval);
		clientApp.Start(MilliSeconds(0 + random));
		//clientApp.Stop(Seconds(simulationTime + 1));
	}
}

void configureTCPIPCameraServer() {
	ObjectFactory factory;
	factory.SetTypeId(TCPIPCameraServer::GetTypeId());
	factory.Set("Port", UintegerValue(82));

	Ptr<Application> tcpServer = factory.Create<TCPIPCameraServer>();
	wifiApNode.Get(0)->AddApplication(tcpServer);

	auto serverApp = ApplicationContainer(tcpServer);
	wireTCPServer(serverApp);
	serverApp.Start(Seconds(0));
//	serverApp.Stop(Seconds(config.simulationTime));
}

void configureTCPIPCameraClients() {

	ObjectFactory factory;
	factory.SetTypeId(TCPIPCameraClient::GetTypeId());
	factory.Set("MotionPercentage",
			DoubleValue(config.ipcameraMotionPercentage));
	factory.Set("MotionDuration",
			TimeValue(Seconds(config.ipcameraMotionDuration)));
	factory.Set("DataRate", UintegerValue(config.ipcameraDataRate));

	factory.Set("PacketSize", UintegerValue(config.payloadSize));
	

	factory.Set("RemoteAddress",
			Ipv4AddressValue(apNodeInterface.GetAddress(0)));
	factory.Set("RemotePort", UintegerValue(82));

	Ptr<UniformRandomVariable> m_rv = CreateObject<UniformRandomVariable>();

	for (uint16_t i = 0; i < config.Nsta; i++) {

		Ptr<Application> tcpClient = factory.Create<TCPIPCameraClient>();
		wifiStaNode.Get(i)->AddApplication(tcpClient);
		auto clientApp = ApplicationContainer(tcpClient);
		wireTCPClient(clientApp, i);

		clientApp.Start(MilliSeconds(0));
		//clientApp.Stop(Seconds(config.simulationTime));
	}
}

void configureTCPFirmwareServer() {
	ObjectFactory factory;
	factory.SetTypeId(TCPFirmwareServer::GetTypeId());
	factory.Set("Port", UintegerValue(83));

	factory.Set("FirmwareSize", UintegerValue(config.firmwareSize));
	factory.Set("BlockSize", UintegerValue(config.firmwareBlockSize));
	factory.Set("NewUpdateProbability",
			DoubleValue(config.firmwareNewUpdateProbability));

	Ptr<Application> tcpServer = factory.Create<TCPFirmwareServer>();
	wifiApNode.Get(0)->AddApplication(tcpServer);

	auto serverApp = ApplicationContainer(tcpServer);
	wireTCPServer(serverApp);
	serverApp.Start(Seconds(0));
//	serverApp.Stop(Seconds(config.simulationTime));
}

void configureTCPFirmwareClients() {

	ObjectFactory factory;
	factory.SetTypeId(TCPFirmwareClient::GetTypeId());
	factory.Set("CorruptionProbability",
			DoubleValue(config.firmwareCorruptionProbability));
	factory.Set("VersionCheckInterval",
			TimeValue(MilliSeconds(config.firmwareVersionCheckInterval)));
	factory.Set("PacketSize", UintegerValue(config.payloadSize));

	factory.Set("RemoteAddress",
			Ipv4AddressValue(apNodeInterface.GetAddress(0)));
	factory.Set("RemotePort", UintegerValue(83));

	Ptr<UniformRandomVariable> m_rv = CreateObject<UniformRandomVariable>();

	for (uint16_t i = 0; i < config.Nsta; i++) {

		Ptr<Application> tcpClient = factory.Create<TCPFirmwareClient>();
		wifiStaNode.Get(i)->AddApplication(tcpClient);
		auto clientApp = ApplicationContainer(tcpClient);
		wireTCPClient(clientApp, i);

		double random = m_rv->GetValue(0, config.trafficInterval);
		clientApp.Start(MilliSeconds(0 + random));
		clientApp.Stop(Seconds(config.simulationTime));
	}
}

void configureTCPSensorServer() {
	/*
	ApplicationContainer sinkApplications;

	auto ipv4 = wifiApNode.Get (0)->GetObject<Ipv4> ();
    const auto address = ipv4->GetAddress (1, 0).GetLocal ();
    InetSocketAddress sinkSocket (address, 84);
    PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", sinkSocket);
    sinkApplications.Add (packetSinkHelper.Install (wifiApNode.Get (0)));
	*/
	
	ObjectFactory factory;
	factory.SetTypeId(TCPSensorServer::GetTypeId());
	factory.Set("Port", UintegerValue(84));

	Ptr<Application> tcpServer = factory.Create<TCPSensorServer>();
	wifiApNode.Get(0)->AddApplication(tcpServer);

	auto serverApp = ApplicationContainer(tcpServer);
	wireTCPServer(serverApp);
	serverApp.Start(Seconds(0));

	//std::cout << "Nsta = " << config.Nsta << std::endl;
	//serverApp.Stop(Seconds(config.simulationTime));
}

void configureTCPSensorClients() {
	/*
	UdpClientHelper client (apNodeInterface.GetAddress(0), 84);
    //client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
    client.SetAttribute ("Interval", TimeValue (MilliSeconds(config.trafficInterval)));
    client.SetAttribute ("PacketSize", UintegerValue (config.sensorMeasurementSize));

    Ptr<UniformRandomVariable> m_rv = CreateObject<UniformRandomVariable>();
	double value = m_rv->GetValue (0, config.trafficInterval);

    //double value = 3;

    std::cout << value << std::endl;
	ApplicationContainer clientApps;

	for (uint16_t i = 0; i < config.Nsta; i++) {
		clientApps = client.Install (wifiStaNode.Get (i));
		clientApps.Start (Seconds (value));
		clientApps.Stop (Seconds (config.simulationTime+value));
	}
	*/

	ObjectFactory factory;
	factory.SetTypeId(TCPSensorClient::GetTypeId());

	factory.Set("Interval", TimeValue(MilliSeconds(config.trafficInterval)));
	std::cout << "interval " << config.trafficInterval << std::endl;
	factory.Set("PacketSize", UintegerValue(config.payloadSize));
	factory.Set("MeasurementSize", UintegerValue(config.sensorMeasurementSize));

	factory.Set("RemoteAddress",
			Ipv4AddressValue(apNodeInterface.GetAddress(0)));
	factory.Set("RemotePort", UintegerValue(84));

	Ptr<UniformRandomVariable> m_rv = CreateObject<UniformRandomVariable>();

	//std::cout << "Nsta = " << config.Nsta << std::endl;

	for (uint16_t i = 0; i < config.Nsta; i++) {

		Ptr<Application> tcpClient = factory.Create<TCPSensorClient>();
		wifiStaNode.Get(i)->AddApplication(tcpClient);
		auto clientApp = ApplicationContainer(tcpClient);
		wireTCPClient(clientApp, i);

		double random = m_rv->GetValue(0, config.trafficInterval);
		clientApp.Start(MilliSeconds(0 + random));
		clientApp.Stop(Seconds(config.simulationTime));
	}
}

void wireTCPServer(ApplicationContainer serverApp) {
	serverApp.Get(0)->TraceConnectWithoutContext("Rx",
			MakeCallback(&tcpPacketReceivedAtServer));
	serverApp.Get(0)->TraceConnectWithoutContext("Retransmission",
			MakeCallback(&tcpRetransmissionAtServer));
	serverApp.Get(0)->TraceConnectWithoutContext("PacketDropped",
			MakeCallback(&tcpPacketDroppedAtServer));
	serverApp.Get(0)->TraceConnectWithoutContext("TCPStateChanged",
			MakeCallback(&tcpStateChangeAtServer));

	if (config.trafficType == "tcpipcamera") {
		serverApp.Get(0)->TraceConnectWithoutContext("DataReceived",
				MakeCallback(&tcpIPCameraDataReceivedAtServer));
	}
}

void wireTCPClient(ApplicationContainer clientApp, int i) {

	clientApp.Get(0)->TraceConnectWithoutContext("Tx",
			MakeCallback(&NodeEntry::OnTcpPacketSent, nodes[i]));
	clientApp.Get(0)->TraceConnectWithoutContext("Rx",
			MakeCallback(&NodeEntry::OnTcpEchoPacketReceived, nodes[i]));

	clientApp.Get(0)->TraceConnectWithoutContext("CongestionWindow",
			MakeCallback(&NodeEntry::OnTcpCongestionWindowChanged, nodes[i]));
	clientApp.Get(0)->TraceConnectWithoutContext("RTO",
			MakeCallback(&NodeEntry::OnTcpRTOChanged, nodes[i]));
	clientApp.Get(0)->TraceConnectWithoutContext("RTT",
			MakeCallback(&NodeEntry::OnTcpRTTChanged, nodes[i]));
	clientApp.Get(0)->TraceConnectWithoutContext("SlowStartThreshold",
			MakeCallback(&NodeEntry::OnTcpSlowStartThresholdChanged, nodes[i]));
	clientApp.Get(0)->TraceConnectWithoutContext("EstimatedBW",
			MakeCallback(&NodeEntry::OnTcpEstimatedBWChanged, nodes[i]));

	clientApp.Get(0)->TraceConnectWithoutContext("TCPStateChanged",
			MakeCallback(&NodeEntry::OnTcpStateChanged, nodes[i]));
	clientApp.Get(0)->TraceConnectWithoutContext("Retransmission",
			MakeCallback(&NodeEntry::OnTcpRetransmission, nodes[i]));

	clientApp.Get(0)->TraceConnectWithoutContext("PacketDropped",
			MakeCallback(&NodeEntry::OnTcpPacketDropped, nodes[i]));

	if (config.trafficType == "tcpfirmware") {
		clientApp.Get(0)->TraceConnectWithoutContext("FirmwareUpdated",
				MakeCallback(&NodeEntry::OnTcpFirmwareUpdated, nodes[i]));
	} else if (config.trafficType == "tcpipcamera") {
		clientApp.Get(0)->TraceConnectWithoutContext("DataSent",
				MakeCallback(&NodeEntry::OnTcpIPCameraDataSent, nodes[i]));
		clientApp.Get(0)->TraceConnectWithoutContext("StreamStateChanged",
				MakeCallback(&NodeEntry::OnTcpIPCameraStreamStateChanged,
						nodes[i]));
	}
}

void configureTCPEchoClients() {
	TcpEchoClientHelper clientHelper(apNodeInterface.GetAddress(0), 80); //address of remote node
	clientHelper.SetAttribute("MaxPackets", UintegerValue(4294967295u));
	clientHelper.SetAttribute("Interval", TimeValue(MilliSeconds(config.trafficInterval)));
	//clientHelper.SetAttribute("IntervalDeviation", TimeValue(MilliSeconds(config.trafficIntervalDeviation)));
	clientHelper.SetAttribute("PacketSize", UintegerValue(config.payloadSize));

	Ptr<UniformRandomVariable> m_rv = CreateObject<UniformRandomVariable>();
	
	for (uint16_t i = 0; i < config.Nsta; i++) {
		ApplicationContainer clientApp = clientHelper.Install(
				wifiStaNode.Get(i));
		wireTCPClient(clientApp, i);

		double random = m_rv->GetValue(0, config.trafficInterval);
		clientApp.Start(MilliSeconds(0 + random));
		//clientApp.Stop(Seconds(simulationTime + 1));
	}
}

void configureUDPClients() {
	//Application start time
	Ptr<UniformRandomVariable> m_rv = CreateObject<UniformRandomVariable>();

	UdpClientHelper myClient(apNodeInterface.GetAddress(0), 9); //address of remote node
	myClient.SetAttribute("MaxPackets", config.maxNumberOfPackets);
	myClient.SetAttribute("PacketSize", UintegerValue(config.payloadSize));
	traffic_sta.clear();
	ifstream trafficfile(config.TrafficPath);
	if (trafficfile.is_open()) {
		uint16_t sta_id;
		float sta_traffic;
		for (uint16_t kk = 0; kk < config.Nsta; kk++) {
			trafficfile >> sta_id;
			trafficfile >> sta_traffic;
			traffic_sta.insert(std::make_pair(sta_id, sta_traffic)); //insert data
			cout << "sta_id = " << sta_id << " sta_traffic = " << sta_traffic << "\n";
		}
		trafficfile.close();
	} else
		cout << "Unable to open traffic file \n";

	double randomStart = 0.0;
	for (std::map<uint16_t, float>::iterator it = traffic_sta.begin();
			it != traffic_sta.end(); ++it) {
		std::ostringstream intervalstr;
		intervalstr << (config.payloadSize * 8) / (it->second * 1000000);
		std::string intervalsta = intervalstr.str();
        cout << "sta_id = " << it->first << " intervalsta = " << intervalsta << "\n";
		//config.trafficInterval = UintegerValue (Time (intervalsta));

		myClient.SetAttribute("Interval", TimeValue(Time(intervalsta))); // TODO add to nodeEntry and visualize
		randomStart = m_rv->GetValue(0,
				(config.payloadSize * 8) / (it->second * 1000000));
		ApplicationContainer clientApp = myClient.Install(
				wifiStaNode.Get(it->first));
		clientApp.Get(0)->TraceConnectWithoutContext("Tx",
				MakeCallback(&NodeEntry::OnUdpPacketSent, nodes[it->first]));
		clientApp.Start(Seconds(1 + randomStart));
		//clientApp.Stop (Seconds (config.simulationTime+1)); //with this throughput is smaller
	}
	AppStartTime = Simulator::Now().GetSeconds() + 1;
	//Simulator::Stop (Seconds (config.simulationTime+1));
}

void configureUDPEchoClients() {
	/* Probing */
	uint32_t packetSizeEcho = 50;
 	uint32_t maxPacketCount = 200000;
 	Time echoInterPacketInterval = Seconds (2.);

  	uint16_t port = 20, echoPort = 10; // well-known echo port number
  
    UdpEchoServerHelper echoServer(echoPort); // Port # 10
    ApplicationContainer echoServerApps = echoServer.Install(wifiApNode.Get(0));
    echoServerApps.Start(Seconds(0.0));
    echoServerApps.Stop(Seconds(config.simulationTime+1));

    // UDP Echo Client application to be installed in the probing station
    Address serverAddress = Address(apNodeInterface.GetAddress (0));
    UdpEchoClientHelper echoClient1(serverAddress, echoPort);
    
    echoClient1.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    echoClient1.SetAttribute("Interval", TimeValue(echoInterPacketInterval));
    echoClient1.SetAttribute("PacketSize", UintegerValue(packetSizeEcho));

    ApplicationContainer clientApps1 = echoClient1.Install(wifiStaNode.Get(0));
    clientApps1.Start(Seconds(0.0));
    clientApps1.Stop(Seconds(config.simulationTime+1));
	

	UdpClientHelper clientHelper(apNodeInterface.GetAddress(0), 9); //address of remote node
	clientHelper.SetAttribute("MaxPackets", UintegerValue(4294967295u));
	clientHelper.SetAttribute("Interval", TimeValue(Seconds(config.trafficInterval)));
	//clientHelper.SetAttribute("IntervalDeviation", TimeValue(MilliSeconds(config.trafficIntervalDeviation)));
	clientHelper.SetAttribute("PacketSize", UintegerValue(config.payloadSize));

	Ptr<UniformRandomVariable> m_rv = CreateObject<UniformRandomVariable>();

	for (uint16_t i = 1; i < config.Nsta; i++) {
		clientApp = clientHelper.Install(
				wifiStaNode.Get(i));
		clientApp.Get(0)->TraceConnectWithoutContext("Tx",
				MakeCallback(&NodeEntry::OnUdpPacketSent, nodes[i]));
		clientApp.Get(0)->TraceConnectWithoutContext("Rx",
				MakeCallback(&NodeEntry::OnUdpEchoPacketReceived, nodes[i]));

		double random = m_rv->GetValue(0, config.trafficInterval);

		//std::cout << random << std::endl;

		clientApp.Start(Seconds(1 + random));
		clientApp.Stop(Seconds(config.trafficInterval + config.simulationTime));
		anthonyApp.Add(clientApp);
	}
}

void configureCameraClients() {
	InetSocketAddress sinkSocket (apNodeInterface.GetAddress(0), 9);
	OnOffHelper clientHelper ("ns3::UdpSocketFactory", sinkSocket);
    clientHelper.SetAttribute ("PacketSize", UintegerValue (config.payloadSize));
    clientHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    clientHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    clientHelper.SetAttribute ("DataRate", DataRateValue (DataRate (config.datarate+"Mbps")));

	Ptr<UniformRandomVariable> m_rv = CreateObject<UniformRandomVariable>();

	for (uint16_t i = 0; i < config.Nsta; i++) {
		clientApp = clientHelper.Install(
				wifiStaNode.Get(i));
		clientApp.Get(0)->TraceConnectWithoutContext("Tx",
				MakeCallback(&NodeEntry::OnUdpPacketSent, nodes[i]));
		clientApp.Get(0)->TraceConnectWithoutContext("Rx",
				MakeCallback(&NodeEntry::OnUdpEchoPacketReceived, nodes[i]));

		double random = m_rv->GetValue(0, config.trafficInterval);

		clientApp.Start(MilliSeconds(1 + random));
		clientApp.Stop(Seconds(config.trafficInterval + config.simulationTime));
		anthonyApp.Add(clientApp);
	}
}

Time timeIdleArray[MaxSta];
Time timeRxArray[MaxSta];
Time timeTxArray[MaxSta];
Time timeSleepArray[MaxSta];
Time timeCollisionArray[MaxSta];

Time timeIdleNotAssociated[MaxSta];
Time timeRxNotAssociated[MaxSta];
Time timeTxNotAssociated[MaxSta];
Time timeSleepNotAssociated[MaxSta];
Time timeCollisionNotAssociated[MaxSta];

double dist[MaxSta];

//it prints the information regarding the state of the device
void PhyStateTrace(std::string context, Time start, Time duration,
		enum WifiPhy::State state) {

	/*Get the number of the node from the context*/
	/*context = "/NodeList/"+strSTA+"/DeviceList/'*'/Phy/$ns3::YansWifiPhy/State/State"*/
	unsigned first = context.find("t/");
	unsigned last = context.find("/D");
	string strNew = context.substr((first + 2), (last - first - 2));

	int node = std::stoi(strNew);

	if (nodes[node]->isAssociated)
	{
		switch (state)
		{
		case WifiPhy::State::SLEEP: //Sleep
			timeSleepArray[node] = timeSleepArray[node] + duration;
			//NS_LOG_UNCOND(to_string(node + 1) + ",SLEEP," + to_string(start.GetMicroSeconds()) + " " + to_string(duration.GetMicroSeconds()));
			break;
		case WifiPhy::State::IDLE: //Idle
			timeIdleArray[node] = timeIdleArray[node] + duration;
			//NS_LOG_UNCOND(to_string(node + 1) + ",IDLE," + to_string(start.GetMicroSeconds()) + " " + to_string(duration.GetMicroSeconds()));
			break;
		case WifiPhy::State::TX: //Tx
			timeTxArray[node] = timeTxArray[node] + duration;
			//NS_LOG_UNCOND (to_string(node+1) + ",TX," + to_string(start.GetMicroSeconds()) + " " + to_string(duration.GetMicroSeconds()));
			break;
		case WifiPhy::State::RX: //Rx
			timeRxArray[node] = timeRxArray[node] + duration;
			//NS_LOG_UNCOND (to_string(node+1) + ",RX," + to_string(start.GetMicroSeconds()) + " " + to_string(duration.GetMicroSeconds()));
			break;
		case WifiPhy::State::CCA_BUSY: //CCA_BUSY
			timeCollisionArray[node] = timeCollisionArray[node] + duration;
			//NS_LOG_UNCOND (to_string(node+1) + ",CCA_BUSY," + to_string(start.GetMicroSeconds()) + " " + to_string(duration.GetMicroSeconds()));
			break;
		}
	}
	else
	{
		switch (state)
		{
		case WifiPhy::State::SLEEP: //Sleep
			timeSleepNotAssociated[node] = timeSleepNotAssociated[node] + duration;
			//NS_LOG_UNCOND(to_string(node + 1) + ",SLEEP," + to_string(start.GetMicroSeconds()) + " " + to_string(duration.GetMicroSeconds()));
			break;
		case WifiPhy::State::IDLE: //Idle
			timeIdleNotAssociated[node] = timeIdleNotAssociated[node] + duration;
			//NS_LOG_UNCOND(to_string(node + 1) + ",IDLE," + to_string(start.GetMicroSeconds()) + " " + to_string(duration.GetMicroSeconds()));
			break;
		case WifiPhy::State::TX: //Tx
			timeTxNotAssociated[node] = timeTxNotAssociated[node] + duration;
			//NS_LOG_UNCOND (to_string(node+1) + ",TX," + to_string(start.GetMicroSeconds()) + " " + to_string(duration.GetMicroSeconds()));
			break;
		case WifiPhy::State::RX: //Rx
			timeRxNotAssociated[node] = timeRxNotAssociated[node] + duration;
			//NS_LOG_UNCOND (to_string(node+1) + ",RX," + to_string(start.GetMicroSeconds()) + " " + to_string(duration.GetMicroSeconds()));
			break;
		case WifiPhy::State::CCA_BUSY: //CCA_BUSY
			timeCollisionNotAssociated[node] = timeCollisionNotAssociated[node] + duration;
			//NS_LOG_UNCOND (to_string(node+1) + ",CCA_BUSY," + to_string(start.GetMicroSeconds()) + " " + to_string(duration.GetMicroSeconds()));
			break;
		}
	}
}

int main(int argc, char *argv[]) {
	SeedManager::SetSeed (3);  // Changes seed from default of 1 to 3
  SeedManager::SetRun (7);  // Changes run number from default of 1 to 7
	/*
    uint32_t nDevices = 1; // 1 PAN Node + 1 Probing node + (nDevices-2) communication nodes
 	uint32_t nGW = 1;
	double batteryCapacity = 5200;
	double voltage = 12;
	double simulationTime = 10;
  	double distance = 2000;
  	double period = 1;
	string dataRate = "100";
	string mcs="0";
  	uint32_t packetSize = 100; //Bytes
	string propLoss = "OkumuraHataPropagationLossModel";
	*/

	uint32_t BeaconInterval = 102400;
	int NRawSta;
	uint32_t pagePeriod=2;  	//  Number of Beacon Intervals between DTIM beacons that carry Page Slice element for the associated page
	uint8_t pageIndex = 0;
	uint32_t pageSliceLength=6; //  Number of blocks in each TIM for the associated page except for the last TIM (1-31) (value 0 is reserved);
	uint32_t pageSliceCount=2;  //  Number of TIMs in a single page period (1-31)
	uint8_t blockOffset = 0;  	//  The 1st page slice starts with the block with blockOffset number
	uint8_t timOffset = 0;    	//  Offset in number of Beacon Intervals from the DTIM that carries the first page slice of the page

	/*
	CommandLine cmd;
	cmd.AddValue ("nDevices", "Number of stations", nDevices);
	cmd.AddValue ("distance", "Distance between EDs and PAN", distance);
	cmd.AddValue ("simulationTime", "Simulation time", simulationTime);
	cmd.AddValue ("period", "Packet period", period);
	cmd.AddValue ("packetSize", "Packet size", packetSize);
	cmd.AddValue ("beaconInterval", "Beacon Interval", BeaconInterval);
	cmd.AddValue ("NRawSta", "NRaw Sta", NRawSta);
	cmd.AddValue ("bandwidth", "Bandwidth", bandWidth);
	cmd.Parse (argc, argv);
	*/

	LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
	LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
	LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
	LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
	LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
	//LogComponentEnable ("YansWifiPhy", LOG_LEVEL_ALL);

	LogComponentEnableAll (LOG_PREFIX_FUNC);
  	LogComponentEnableAll (LOG_PREFIX_NODE);
  	LogComponentEnableAll (LOG_PREFIX_TIME);

	//LogComponentEnable ("ApWifiMac", LOG_DEBUG);
	//LogComponentEnable ("StaWifiMac", LOG_DEBUG);
	//LogComponentEnable ("EdcaTxopN", LOG_DEBUG);

	bool OutputPosition = false;
	config = Configuration(argc, argv);

	config.Nsta = (int) config.Nsta / config.nGW;
	config.rho = config.rho / config.nGW;

	config.rps = configureRAW(config.rps, config.RAWConfigFile);
	//config.Nsta = config.NRawSta;

	/*
	config.trafficInterval = period;
	config.Nsta = nDevices;
	config.payloadSize = packetSize;
	config.rho = distance;
	config.datarate = dataRate;
	config.simulationTime = simulationTime;
	config.batteryCapacity = batteryCapacity;
	config.voltage = voltage;
	config.propLoss = propLoss;
	config.mcs = mcs;
	*/

	configurePageSlice ();
	configureTIM ();
	//checkRawAndTimConfiguration ();

	//std::cout << "nsta= " << config.Nsta << std::endl;
	/*
	config.NSSFile = config.trafficType + "_" + std::to_string(config.Nsta)
			+ "sta_" + std::to_string(config.NGroup) + "Group_"
			+ std::to_string(config.NRawSlotNum) + "slots_"
			+ std::to_string(config.payloadSize) + "payload_"
			+ std::to_string(config.totaltraffic) + "Mbps_"
			+ std::to_string(config.BeaconInterval) + "BI" + ".nss";

	eventManager = SimulationEventManager(config.visualizerIP,
			config.visualizerPort, config.NSSFile);
	*/

	stats = Statistics(config.Nsta);	
	uint32_t totalRawGroups(0);
	for (int i = 0; i < config.rps.rpsset.size(); i++) {
		int nRaw = config.rps.rpsset[i]->GetNumberOfRawGroups();
		totalRawGroups += nRaw;
		//cout << "Total raw groups after rps " << i << " is " << totalRawGroups << endl;
		for (int j = 0; j < nRaw; j++) {
			config.totalRawSlots += config.rps.rpsset[i]->GetRawAssigmentObj(j).GetSlotNum();
			//cout << "Total slots after group " << j << " is " << totalRawSlots << endl;
		}

	}
	
	transmissionsPerTIMGroupAndSlotFromAPSinceLastInterval = vector<long>(
			config.totalRawSlots, 0);
	transmissionsPerTIMGroupAndSlotFromSTASinceLastInterval = vector<long>(
			config.totalRawSlots, 0);

	RngSeedManager::SetSeed(config.seed);

	std::cout << "Nsta = " << config.Nsta << " " << config.nGW << std::endl;

	wifiStaNode.Create(config.Nsta);
	wifiApNode.Create(1);

	YansWifiChannelHelper channelBuilder = YansWifiChannelHelper();
	
	//channelBuilder.AddPropagationLoss("ns3::LogDistancePropagationLossModel",
	//		"Exponent", DoubleValue(3.76), "ReferenceLoss", DoubleValue(8.0),
	//		"ReferenceDistance", DoubleValue(1.0));

	std::string propLoss;
	if (config.radioEnvironment == "urban") propLoss = "Cost231PropagationLossModel";
	else if (config.radioEnvironment == "suburban") propLoss = "LogDistancePropagationLossModel";
	else if (config.radioEnvironment == "rural") propLoss = "OkumuraHataPropagationLossModel";
	else if (config.radioEnvironment == "indoor") propLoss = "HybridBuildingsPropagationLossModel";
	
	channelBuilder.AddPropagationLoss("ns3::"+propLoss);
	channelBuilder.SetPropagationDelay(
			"ns3::ConstantSpeedPropagationDelayModel");
	Ptr<YansWifiChannel> channel = channelBuilder.Create();
	channel->TraceConnectWithoutContext("Transmission",
			MakeCallback(&onChannelTransmission)); //TODO

	StringValue DataRate;

	std::string data_mode = "MCS" + config.bandWidth + "_" +  config.mcs;

	std::cout << data_mode << std::endl;

	DataRate = StringValue(getWifiMode(data_mode)); // changed

	YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
	phy.SetErrorRateModel("ns3::YansErrorRateModel");
	phy.SetChannel(channel);
	phy.Set("ShortGuardEnabled", BooleanValue(config.sgi));
	phy.Set("ChannelWidth", UintegerValue((getBandwidth(data_mode)))); // changed
	phy.Set("EnergyDetectionThreshold", DoubleValue(-110.0));
	phy.Set("CcaMode1Threshold", DoubleValue(-113.0));
	phy.Set("TxGain", DoubleValue(0.0));
	phy.Set("RxGain", DoubleValue(0.0));
	phy.Set("TxPowerLevels", UintegerValue(1));
	phy.Set("TxPowerEnd", DoubleValue(0.0));
	phy.Set("TxPowerStart", DoubleValue(0.0));
	phy.Set("RxNoiseFigure", DoubleValue(6.8));
	phy.Set("LdpcEnabled", BooleanValue(true));
	phy.Set("S1g1MfieldEnabled", BooleanValue(config.S1g1MfieldEnabled));

	WifiHelper wifi = WifiHelper::Default();
	wifi.SetStandard(WIFI_PHY_STANDARD_80211ah);
	S1gWifiMacHelper mac = S1gWifiMacHelper::Default();

	Ssid ssid = Ssid("ns380211ah");
  
	//DataRate = StringValue("OfdmRate1_3MbpsBW2MHz");

	if (config.mcs == "10") {
   		wifi.SetRemoteStationManager("ns3::IdealWifiManager");
  	}
  	else {
		wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode",DataRate, "ControlMode", DataRate);
 		//wifi.SetRemoteStationManager("ns3::MinstrelWifiManager");
	}


	mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing",
			BooleanValue(false));

	NetDeviceContainer staDevice;
	staDevice = wifi.Install(phy, mac, wifiStaNode);

	mac.SetType ("ns3::ApWifiMac",
	                 "Ssid", SsidValue (ssid),
	                 "BeaconInterval", TimeValue (MicroSeconds(config.BeaconInterval)),
	                 "NRawStations", UintegerValue (config.NRawSta),
	                 "RPSsetup", RPSVectorValue (config.rps),
	                 "PageSliceSet", pageSliceValue (config.pageS),
	                 "TIMSet", TIMValue (config.tim)
	               );

	phy.Set("TxGain", DoubleValue(3.0));
	phy.Set("RxGain", DoubleValue(3.0));
	phy.Set("TxPowerLevels", UintegerValue(1));
	phy.Set("TxPowerEnd", DoubleValue(30.0));
	phy.Set("TxPowerStart", DoubleValue(30.0));
	phy.Set("RxNoiseFigure", DoubleValue(6.8));
    phy.Set ("ChannelWidth", UintegerValue (4));

	apDevice = wifi.Install(phy, mac, wifiApNode);

	Config::Set(
			"/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/MaxPacketNumber",
			UintegerValue(10));
	Config::Set(
			"/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/MaxDelay",
			TimeValue(NanoSeconds(6000000000000)));

	std::ostringstream oss;
	oss << "/NodeList/" << wifiApNode.Get(0)->GetId()
			<< "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::ApWifiMac/";
	Config::ConnectWithoutContext(oss.str() + "RpsIndex", MakeCallback(&RpsIndexTrace));
	Config::ConnectWithoutContext(oss.str() + "RawGroup", MakeCallback(&RawGroupTrace));
	Config::ConnectWithoutContext(oss.str() + "RawSlot", MakeCallback(&RawSlotTrace));

	// mobility.
    /* 
	MobilityHelper mobility;
	double xpos = std::stoi(config.rho, nullptr, 0);
	double ypos = xpos;
	mobility.SetPositionAllocator("ns3::UniformDiscPositionAllocator", "X",
			StringValue(std::to_string(xpos)), "Y",
			StringValue(std::to_string(ypos)), "rho", StringValue(config.rho));
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.Install(wifiStaNode);
    */
    
    MobilityHelper mobility;
	
   // Ptr<ListPositionAllocator> position = CreateObject<ListPositionAllocator> ();
    //position->Add (Vector (0, 0, 0));
	if (config.mobileNodes) {
		/*
		mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
										"MinX", DoubleValue (0.0),
										"MinY", DoubleValue (0.0),
										"DeltaX", DoubleValue (10.0),
										"DeltaY", DoubleValue (10.0),
										"GridWidth", UintegerValue (3),
										"LayoutType", StringValue ("RowFirst"));
		*/
		mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator", "rho", DoubleValue (config.rho),
                                 "X", DoubleValue (0.0), "Y", DoubleValue (0.0), "Z", DoubleValue(1.0));
		
		//mobility.SetPositionAllocator (position);
		mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
									"Mode", StringValue ("Time"),
									"Time", StringValue (to_string(config.simulationTime)),
									"Speed", StringValue ("ns3::ConstantRandomVariable[Constant=10.0]"),
									"Bounds", RectangleValue (Rectangle (-config.rho, config.rho, -config.rho, config.rho)));
		mobility.Install(wifiStaNode);
		mobility.AssignStreams (wifiStaNode, 0);
		PrintPositions (wifiStaNode);
	}
	else {
		if (config.radioEnvironment == "indoor") {
			double x_min = 0.0;
			double x_max = 20.0;
			double y_min = 0.0;
			double y_max = 10.0;
			double z_min = 0.0;
			double z_max = 50.0;

			double _x_max = x_max / config.nGW;
			double _y_max = y_max / config.nGW;
			double _z_max = z_max / config.nGW;

			std::string m_x = "ns3::UniformRandomVariable[Min=0.0|Max="+std::to_string(_x_max)+"]";
			std::string m_y = "ns3::UniformRandomVariable[Min=0.0|Max="+std::to_string(_y_max)+"]";
			std::string m_z = "ns3::UniformRandomVariable[Min=0.0|Max="+std::to_string(_z_max)+"]";

			mobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator", "X", StringValue (m_x),
											"Y", StringValue (m_y), "Z", StringValue (m_z));
			mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
			mobility.Install(wifiStaNode);

			MobilityHelper mobilityAp;
			Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
			positionAlloc->Add (Vector (_x_max/2, _y_max/2, _z_max/2));
			mobilityAp.SetPositionAllocator (positionAlloc);
			mobilityAp.SetMobilityModel("ns3::ConstantPositionMobilityModel");
			mobilityAp.Install(wifiApNode);

			//std::cout << z_max << std::endl;
			//double z_max = 50.0;
			Ptr<Building> b = CreateObject <Building> ();
			b->SetBoundaries (Box (x_min, x_max, y_min, y_max, z_min, z_max));
			b->SetBuildingType (Building::Residential);
			b->SetExtWallsType (Building::ConcreteWithWindows);
			b->SetNFloors (16);
			b->SetNRoomsX (3);
			b->SetNRoomsY (2);

			BuildingsHelper::Install (wifiStaNode);
			
			BuildingsHelper::Install (wifiApNode);
			}
		else {
			mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator", "rho", DoubleValue (config.rho),
											"X", DoubleValue (0.0), "Y", DoubleValue (0.0), "Z", DoubleValue(1.0));
			mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
			mobility.Install(wifiStaNode);

			MobilityHelper mobilityAp;
			Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
			positionAlloc->Add (Vector (0, 0, 1));
			mobilityAp.SetPositionAllocator (positionAlloc);
			mobilityAp.SetMobilityModel("ns3::ConstantPositionMobilityModel");
			mobilityAp.Install(wifiApNode);
		}
  	}

	/* Internet stack*/
	InternetStackHelper stack;
	stack.Install(wifiApNode);
	stack.Install(wifiStaNode);

	Ipv4AddressHelper address;

	address.SetBase("192.168.0.0", "255.255.0.0");

	staNodeInterface = address.Assign(staDevice);
	apNodeInterface = address.Assign(apDevice);

	//trace association
	for (uint16_t kk = 0; kk < config.Nsta; kk++) {
		std::ostringstream STA;
		STA << kk;
		std::string strSTA = STA.str();

		assoc_record *m_assocrecord = new assoc_record;
		m_assocrecord->setstaid(kk);
		Config::Connect(
				"/NodeList/" + strSTA
						+ "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/Assoc",
				MakeCallback(&assoc_record::SetAssoc, m_assocrecord));
		Config::Connect(
				"/NodeList/" + strSTA
						+ "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/DeAssoc",
				MakeCallback(&assoc_record::UnsetAssoc, m_assocrecord));
		assoc_vector.push_back(m_assocrecord);
	}

	std::cout << "Populating routing tables..." << std::endl;
	Ipv4GlobalRoutingHelper::PopulateRoutingTables();
	std::cout << "Populating ARP cache..." << std::endl;
	PopulateArpCache();

	// configure tracing for associations & other metrics
	std::cout << "Configuring trace sinks for nodes..." << std::endl;
	configureNodes(wifiStaNode, staDevice);

	Config::Connect(
			"/NodeList/" + std::to_string(config.Nsta)
					+ "/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyRxDropWithReason",
			MakeCallback(&OnAPPhyRxDrop));
	Config::Connect(
			"/NodeList/" + std::to_string(config.Nsta)
					+ "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::ApWifiMac/PacketToTransmitReceivedFromUpperLayer",
			MakeCallback(&OnAPPacketToTransmitReceived));

	Ptr<MobilityModel> mobility1 =
			wifiApNode.Get(0)->GetObject<MobilityModel>();
	Vector apposition = mobility1->GetPosition();
	if (OutputPosition) {
		uint32_t i = 0;
		while (i < config.Nsta) {
			Ptr<MobilityModel> mobility = wifiStaNode.Get(i)->GetObject<
					MobilityModel>();
			Vector position = mobility->GetPosition();
			nodes[i]->x = position.x;
			nodes[i]->y = position.y;
			std::cout << "Sta node#" << i << ", " << "position = " << position
					<< std::endl;
			dist[i] = mobility->GetDistanceFrom(
					wifiApNode.Get(0)->GetObject<MobilityModel>());
			i++;
		}
		std::cout << "AP node, position = " << apposition << std::endl;
	}

	/*Print of the state of the stations*/
	for (uint32_t i = 0; i < config.Nsta; i++) {
		std::ostringstream STA;
		STA << i;
		std::string strSTA = STA.str();

		Config::Connect(
				"/NodeList/" + strSTA
						+ "/DeviceList/*/Phy/$ns3::YansWifiPhy/State/State",
				MakeCallback(&PhyStateTrace));
	}

	eventManager.onStartHeader();
	eventManager.onStart(config);
	if (config.rps.rpsset.size() > 0)
		for (uint32_t i = 0; i < config.rps.rpsset.size(); i++)
			for (uint32_t j = 0;
					j < config.rps.rpsset[i]->GetNumberOfRawGroups(); j++)
				eventManager.onRawConfig(i, j,
						config.rps.rpsset[i]->GetRawAssigmentObj(j));

	for (uint32_t i = 0; i < config.Nsta; i++)
		eventManager.onSTANodeCreated(*nodes[i]);

	eventManager.onAPNodeCreated(apposition.x, apposition.y);
	eventManager.onStatisticsHeader();

	sendStatistics(true);

	Simulator::Stop(Seconds(config.simulationTime + config.CoolDownPeriod)); // allow up to a minute after the client & server apps are finished to process the queue
	Simulator::Run();

	// Visualizer throughput
	/*
	int pay = 0, totalSuccessfulPackets = 0, totalSentPackets = 0, totalPacketsEchoed = 0;
	for (int i = 0; i < config.Nsta; i++)
	{
		totalSuccessfulPackets += stats.get(i).NumberOfSuccessfulPackets;
		totalSentPackets += stats.get(i).NumberOfSentPackets;
		totalPacketsEchoed += stats.get(i).NumberOfSuccessfulRoundtripPackets;
		pay += stats.get(i).TotalPacketPayloadSize;
		/*
		cout << i << " sent: " << stats.get(i).NumberOfSentPackets
				<< " ; delivered: " << stats.get(i).NumberOfSuccessfulPackets
				<< " ; echoed: " << stats.get(i).NumberOfSuccessfulRoundtripPackets
				<< "; packetloss: "
				<< stats.get(i).GetPacketLoss(config.trafficType) << endl;
		
	}
	*/

	double wholeSuccessRate, throughput;

	if (config.trafficType == "udpcamera")
	{
		double totalPacketsThrough = 0;
		for (uint32_t index = 0; index < sinkApplications.GetN (); ++index) {
			totalPacketsThrough = DynamicCast<PacketSink> (sinkApplications.Get (index)) ->GetTotalRx ();
			throughput += ((totalPacketsThrough * 8) / ((config.simulationTime) * 1024)); //Mbit/s
		}

		double totalSent;
		UintegerValue packetsSent;
		for (uint32_t index = 0; index < anthonyApp.GetN (); ++index) {
			anthonyApp.Get (index)->GetAttribute("TotalTx", packetsSent);    
			totalSent = totalSent + packetsSent.Get();
		}

		wholeSuccessRate =  (totalPacketsThrough / totalSent) * 100;
		
		wholeSuccessRate = std::min (wholeSuccessRate, 100.0);
	}
	else if (config.trafficType == "udpecho")
	{
		/*
		double ulThroughput = 0, dlThroughput = 0;
		ulThroughput = totalSuccessfulPackets * config.payloadSize * 8 / (config.simulationTime * 1000000.0);
		dlThroughput = totalPacketsEchoed * config.payloadSize * 8 / (config.simulationTime * 1000000.0);

		
		cout << "totalPacketsSent " << totalSentPackets << endl;
		cout << "totalPacketsDelivered " << totalSuccessfulPackets << endl;
		cout << "totalPacketsEchoed " << totalPacketsEchoed << endl;
		cout << "UL packets lost " << totalSentPackets - totalSuccessfulPackets << endl;
		cout << "DL packets lost " << totalSuccessfulPackets - totalPacketsEchoed << endl;
		cout << "Total packets lost " << totalSentPackets - totalPacketsEchoed << endl;
		*/

		/*cout << "uplink throughput Mbit/s " << ulThroughput << endl;
		cout << "downlink throughput Mbit/s " << dlThroughput << endl;*/

		//double throughput = (totalSuccessfulPackets + totalPacketsEchoed) * config.payloadSize * 8 / (config.simulationTime * 1000000.0);
		//cout << "total throughput Kbit/s " << throughput * 1000 << endl;

		//std::cout << "datarate" << "\t" << "throughput" << std::endl;
		//std::cout << config.datarate << "\t" << throughput * 1000 << " Kbit/s" << std::endl;

		double totalPacketsThrough = 0;
		double totalSent;
		for (uint32_t index = 0; index < sinkApplications.GetN (); ++index) {
			totalPacketsThrough = DynamicCast<PacketSink> (sinkApplications.Get (index)) ->GetTotalRx ();
			throughput += ((totalPacketsThrough * 8) / ((config.simulationTime) * 1024)); //Mbit/s
		}

		for (uint32_t index = 0; index < anthonyApp.GetN (); ++index) {
			totalSent += DynamicCast<UdpClient> (anthonyApp.Get (index)) ->GetTotalTx ();
		}
		
		//totalSent *= config.Nsta;

		wholeSuccessRate =  (totalPacketsThrough / totalSent) * 100;
		wholeSuccessRate = std::min (wholeSuccessRate, 100.0);
	}
	//cout << "Success Rate: " << 100. * totalSuccessfulPackets / totalSentPackets << endl;
	Simulator::Destroy();

    ofstream risultati;
    string addressresults = config.OutputPath + "moreinfo.txt";
    risultati.open(addressresults.c_str(), ios::out | ios::trunc);

    risultati << "Sta node#,distance,timerx(notassociated),timeidle(notassociated),timetx(notassociated),timesleep(notassociated),timecollision(notassociated)" << std::endl;
    int i = 0;
    string spazio = ",";
    double capacityJoules = config.batteryCapacity * config.voltage * 3.6;
	double max_energy = 0;

    while (i < config.Nsta) {
        
        risultati << i << spazio << dist[i] << spazio << timeRxArray[i].GetSeconds() << ",(" << timeRxNotAssociated[i].GetSeconds() << ")," << timeIdleArray[i].GetSeconds() << ",(" << timeIdleNotAssociated[i].GetSeconds() << ")," << timeTxArray[i].GetSeconds() << ",(" << timeTxNotAssociated[i].GetSeconds() << ")," << timeSleepArray[i].GetSeconds() << ",(" << timeSleepNotAssociated[i].GetSeconds() << ")," << timeCollisionArray[i].GetSeconds() << ",(" << timeCollisionNotAssociated[i].GetSeconds() << ")" << std::endl;
        /*
         cout << "Sleep " << stats.get(i).TotalSleepTime.GetSeconds() << endl;
         cout << "Tx " << stats.get(i).TotalTxTime.GetSeconds() << endl;
         cout << "Rx " << stats.get(i).TotalRxTime.GetSeconds() << endl;
         cout << "IDLE " << stats.get(i).TotalIdleTime.GetSeconds() << endl;
         cout << "TOTENERGY " <<  stats.get(i).GetTotalEnergyConsumption() << " mW" << endl; // Should be Milli-Joules NodeEntry.269
         cout << "Rx+Idle ENERGY " <<  stats.get(i).EnergyRxIdle << " mW" << endl;
         cout << "Tx ENERGY " <<  stats.get(i).EnergyTx << " mW" << endl;
		*/
		double energyConsumed =  stats.get(i).GetTotalEnergyConsumption() / 1000;
      	max_energy = std::max(energyConsumed, max_energy);
		//std::cout << "simulation time: " << config.simulationTime << " " << config.payloadSize << std::endl;
        i++;
		break;
    }

	std::cout << "Total energy: " << max_energy << std::endl;
	std::cout << "Battery lifetime: " << ((capacityJoules / max_energy) * config.simulationTime) / 86400 << std::endl;
	std::cout << "Throughput: " << throughput << std::endl;
	std::cout << "Success rate: " <<  wholeSuccessRate << std::endl; // Success rate percentage
    
    risultati.close();
	return 0;
}