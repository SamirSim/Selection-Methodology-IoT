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
* Author: Sidharth Nabar <snabar@uw.edu>, He Wu <mdzz@u.washington.edu>
*         Peishuo Li <pressthunder@gmail.com>
*/

#ifndef LR_WPAN_RADIO_ENERGY_MODEL_HELPER_H
#define LR_WPAN_RADIO_ENERGY_MODEL_HELPER_H

#include "ns3/energy-model-helper.h"
#include "ns3/lr-wpan-radio-energy-model.h"

namespace ns3 {

/**
 * \ingroup energy
 * \brief Assign LrWpanRadioEnergyModel to lrwpan devices.
 *
 * This installer installs LrWpanRadioEnergyModel for only LrWpanTschNetDevice objects.
 *
 */
class LrWpanRadioEnergyModelHelper : public DeviceEnergyModelHelper
{
public:
  /**
   * Construct a helper which is used to add a radio energy model to a node
   */
  LrWpanRadioEnergyModelHelper ();

  /**
   * Destroy a RadioEnergy Helper
   */
  ~LrWpanRadioEnergyModelHelper ();

  /**
   * \param name the name of the attribute to set
   * \param v the value of the attribute
   *
   * Sets an attribute of the underlying PHY object.
   */
  void Set (std::string name, const AttributeValue &v);

  /**
   * \param callback Callback function for energy depletion handling.
   *
   * Sets the callback to be invoked when energy is depleted.
   */
  void SetDepletionCallback (
    LrWpanRadioEnergyModel::LrWpanRadioEnergyDepletionCallback callback);

private:
  /**
   * \param device Pointer to the NetDevice to install DeviceEnergyModel.
   * \param source Pointer to EnergySource to install.
   *
   * Implements DeviceEnergyModel::Install. Also track transceiver PHY state
   */
  virtual Ptr<DeviceEnergyModel> DoInstall (Ptr<NetDevice> device,
                                            Ptr<EnergySource> source) const;

private:
  ObjectFactory m_radioEnergy;
  LrWpanRadioEnergyModel::LrWpanRadioEnergyDepletionCallback m_depletionCallback;

};

} // namespace ns3

#endif /* LR_WPAN_RADIO_ENERGY_MODEL_HELPER_H */
