/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Network Security Lab, University of Washington, Seattle.
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
 * Authors: Sidharth Nabar <snabar@uw.edu>, He Wu <mdzz@u.washington.edu>
 *          Peishuo Li <pressthunder@gmail.com>
 */

#include "lr-wpan-radio-energy-model-helper.h"
#include "ns3/lr-wpan-net-device.h"
#include "ns3/lr-wpan-phy.h"
#include "ns3/config.h"
#include "ns3/names.h"
#include "ns3/log.h"

namespace ns3 {

LrWpanRadioEnergyModelHelper::LrWpanRadioEnergyModelHelper ()
{
  m_radioEnergy.SetTypeId ("ns3::LrWpanRadioEnergyModel");
  m_depletionCallback.Nullify ();
}

LrWpanRadioEnergyModelHelper::~LrWpanRadioEnergyModelHelper ()
{
}

void
LrWpanRadioEnergyModelHelper::Set (std::string name, const AttributeValue &v)
{
  m_radioEnergy.Set (name, v);
}

void
LrWpanRadioEnergyModelHelper::SetDepletionCallback (
  LrWpanRadioEnergyModel::LrWpanRadioEnergyDepletionCallback callback)
{
  m_depletionCallback = callback;
}

/*
 * Private function starts here.
 */

Ptr<DeviceEnergyModel>
LrWpanRadioEnergyModelHelper::DoInstall (Ptr<NetDevice> device,
                                       Ptr<EnergySource> source) const
{
  NS_ASSERT (device != NULL);
  NS_ASSERT (source != NULL);

 std::string deviceName = device->GetInstanceTypeId ().GetName ();
 //std::cout << deviceName << std::endl;
 // Changed LrWpanTschNetDevice by LrWpanNetDevice to make the module work for classic lwpan devices
 /*
 if (deviceName.compare ("ns3::LrWpanTschNetDevice") != 0)
   {
     NS_FATAL_ERROR ("NetDevice type is not LrWpanTschNetDevice!");
   }
  */
  Ptr<LrWpanRadioEnergyModel> model = m_radioEnergy.Create ()->GetObject<LrWpanRadioEnergyModel> ();
  NS_ASSERT (model != NULL);

  model->SetEnergySource (source);
  Ptr<LrWpanNetDevice> tschdevice = DynamicCast<LrWpanNetDevice>(device);
  model->SetEnergyDepletionCallback (MakeCallback(&LrWpanPhy::HandleEnergyDepletion, tschdevice->GetPhy()));
  source->AppendDeviceEnergyModel (model);

  //track transceiver state
  Ptr<LrWpanNetDevice> lrWpanTschNetDevice = DynamicCast<LrWpanNetDevice> (device);
  Ptr<LrWpanPhy> lrWpanPhy = lrWpanTschNetDevice->GetPhy ();
  lrWpanPhy -> TraceConnectWithoutContext ("TrxState",MakeCallback(&LrWpanRadioEnergyModel::ChangeLrWpanState, model));

  return model;
}

} // namespace ns3
