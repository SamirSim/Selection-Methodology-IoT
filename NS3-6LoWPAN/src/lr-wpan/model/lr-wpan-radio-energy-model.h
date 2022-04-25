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
 * Authors: Sidharth Nabar <snabar@uw.edu>, He Wu <mdzz@u.washington.edu>,
 *          Peishuo Li <pressthunder@gmail.com>
 */

#ifndef LR_WPAN_RADIO_ENERGY_MODEL_H
#define LR_WPAN_RADIO_ENERGY_MODEL_H

#include "ns3/device-energy-model.h"
#include "ns3/energy-source.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"
#include "ns3/traced-value.h"
#include "lr-wpan-phy.h"
#include <ns3/traced-callback.h>

namespace ns3 {

/**
 * \ingroup energy
 * \brief A LrWpan radio energy model.
 * 
 * 5 states are defined for the radio: TX_ON, BUSY_TX, RX_ON, BUSY_RX, TRX_OFF.
 * Inherit from LrWpanPhyEnumeration
 * The different types of tansactirons that are defined are:
 *  1. TX_ON: Transmitter is enabled.
 *  2. BUSY_TX: Transmitter is busy for transmitting data.
 *  3. RX_ON: Receiver is enabled.
 *  4. BUSY_RX: Receiver is busy for receiving data.
 *  5. TRX_OFF: Transceiver is disabled.
 *  6. TRX_SWITCHING: Transceiver is switching between RX/TX.
 *  7. TRX_START: Transceiver is switching from TRX_OFF to RX/TX.
 * The class keeps track of what state the radio is currently in.
 *
 * Energy calculation: For each transaction, this model notifies EnergySource
 * object. The EnergySource object will query this model for the total current.
 * Then the EnergySource object uses the total current to calculate energy.
 *
 * Default values for power consumption are based on NXP device C40, with
 * supply voltage as 0.9V and currents as 1.5 mA (BUSY_RX), 0.5 mA (RX),
 * 0.5 mA (TRX_SWITCHING), 7 mA (BUSY_TX, TX, @0dBm) and 0.52 uA (TRX_OFF).
 *
 */

class LrWpanRadioEnergyModel : public DeviceEnergyModel
{
public:
  /**
   * Callback type for energy depletion handling.
   */
  typedef Callback<void> LrWpanRadioEnergyDepletionCallback;

public:
  static TypeId GetTypeId (void);
  LrWpanRadioEnergyModel ();
  virtual ~LrWpanRadioEnergyModel ();

  /**
   * \brief Sets pointer to EnergySouce installed on node.
   *
   * \param source Pointer to EnergySource installed on node.
   *
   * Implements DeviceEnergyModel::SetEnergySource.
   */
  virtual void SetEnergySource (Ptr<EnergySource> source);

  /**
   * \returns Total energy consumption of the lrwpan device.
   *
   * Implements DeviceEnergyModel::GetTotalEnergyConsumption.
   */
  virtual double GetTotalEnergyConsumption (void) const;

  // Setter & getters for state power consumption.
  double GetTrxOffCurrentA (void) const;
  void SetTrxOffCurrentA (double TrxOffCurrentA);
  double GetRxBusyCurrentA (void) const;
  void SetRxBusyCurrentA (double RxBusyCurrentA);
  double GetTxCurrentA (void) const;
  void SetTxCurrentA (double txCurrentA);
  double GetRxCurrentA (void) const;
  void SetRxCurrentA (double rxCurrentA);
  double GetTxBusyCurrentA (void) const;
  void SetTxBusyCurrentA (double TxBusyCurrentA);
  double GetTrxSwitchingCurrentA (void) const;
  void SetTrxSwitchingCurrentA (double TrxSwichingCurrentA);
  double GetTrxStartCurrentA (void) const;
  void SetTrxStartCurrentA (double TrxStartCurrentA);

  /**
   * \returns Current state.
   */
  LrWpanPhyEnumeration GetCurrentState (void) const;

  /**
   * \param callback Callback function.
   *
   * Sets callback for energy depletion handling.
   */
  void SetEnergyDepletionCallback (LrWpanRadioEnergyDepletionCallback callback);

  /**
   * \brief Inherit from DeviceEnergyModel, not used
   */
  virtual void ChangeState (int newState);

  /**
   * \brief Handles energy depletion.
   *
   * Implements DeviceEnergyModel::HandleEnergyDepletion
   */
  virtual void HandleEnergyDepletion (void);

  virtual void HandleEnergyChanged (void);

  /**
   * \brief Handles energy recharged.
   *
   * Not implemented
   */
  virtual void HandleEnergyRecharged (void)
  {
  }

  /**
   * \brief Change state of the LrWpanRadioEnergyMode
   */
  void ChangeLrWpanState (Time t, LrWpanPhyEnumeration oldstate, LrWpanPhyEnumeration newstate);

private:
  /**
   * Implement real destruction code and chain up to the parent's implementation once done
   */
  void DoDispose (void);

  /**
   * \returns Current draw of device, at current state.
   *
   * Implements DeviceEnergyModel::GetCurrentA.
   */
  virtual double DoGetCurrentA (void) const;

  /**
   * \param state New state the radio device is currently in.
   *
   * Sets current state. This function is private so that only the energy model
   * can change its own state.
   */
  void SetLrWpanRadioState (const LrWpanPhyEnumeration state);

private:
  Ptr<EnergySource> m_source;

  // Member variables for current draw in different radio modes.
  double m_TxCurrentA;
  double m_RxCurrentA;
  double m_TrxOffCurrentA;
  double m_RxBusyCurrentA;
  double m_TxBusyCurrentA;
  double m_TrxSwitchingCurrentA;
  double m_TrxStartCurrentA;

  // This variable keeps track of the total energy consumed by this model.
  TracedValue<double> m_totalEnergyConsumption;

  TracedCallback<std::string, std::string, bool, double, double, double> m_EnergyStateLogger;

  // State variables.
  LrWpanPhyEnumeration m_currentState;  // current state the radio is in
  Time m_lastUpdateTime;                // time stamp of previous energy update
  double m_energyToDecrease;            // consumed energy of lastest LrWpanRadioEnergyMode
  double m_remainingBatteryEnergy;      // remaining battery energy of the energy source attaching to the node
  bool m_sourceEnergyUnlimited;         // battery energy of the energy source attaching to the node unlimited or not
  bool m_sourcedepleted;                // battery energy of the energy source depleted or not

  // Energy depletion callback
  LrWpanRadioEnergyDepletionCallback m_energyDepletionCallback;

};

} // namespace ns3

#endif /* LR_WPAN_RADIO_ENERGY_MODEL_H */