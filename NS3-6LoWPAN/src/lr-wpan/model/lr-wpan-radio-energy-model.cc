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

#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/simulator.h"
#include "ns3/trace-source-accessor.h"
#include "lr-wpan-radio-energy-model.h"
#include "lr-wpan-energy-source.h"

NS_LOG_COMPONENT_DEFINE ("LrWpanRadioEnergyModel");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (LrWpanRadioEnergyModel);

TypeId
LrWpanRadioEnergyModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LrWpanRadioEnergyModel")
    .SetParent<DeviceEnergyModel> ()
    .AddConstructor<LrWpanRadioEnergyModel> ()
    .AddAttribute ("TrxOffCurrentA",
                   "The default radio Idle current in Ampere.",
                   DoubleValue (0.00000052),
                   MakeDoubleAccessor (&LrWpanRadioEnergyModel::SetTrxOffCurrentA,
                                       &LrWpanRadioEnergyModel::GetTrxOffCurrentA),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("RxBusyCurrentA",
                   "The default radio Rx Busy with data receving current in Ampere.",
                   DoubleValue (0.0015),
                   MakeDoubleAccessor (&LrWpanRadioEnergyModel::SetRxBusyCurrentA,
                                       &LrWpanRadioEnergyModel::GetRxBusyCurrentA),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxCurrentA",
                   "The radio Tx current.",
                   DoubleValue (0.007),
                   MakeDoubleAccessor (&LrWpanRadioEnergyModel::SetTxCurrentA,
                                       &LrWpanRadioEnergyModel::GetTxCurrentA),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("RxCurrentA",
                   "The radio Rx current.",
                   DoubleValue (0.0005),
                   MakeDoubleAccessor (&LrWpanRadioEnergyModel::SetRxCurrentA,
                                       &LrWpanRadioEnergyModel::GetRxCurrentA),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxBusyCurrentA",
                   "The default radio Tx Busy with data transmitting current in Ampere.",
                   DoubleValue (0.007),
                   MakeDoubleAccessor (&LrWpanRadioEnergyModel::SetTxBusyCurrentA,
                                       &LrWpanRadioEnergyModel::GetTxBusyCurrentA),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TrxSwitchingCurrentA",
                   "The default radio Transceiver swiching between Tx/Rx current in Ampere.",
                   DoubleValue (0.0005),
                   MakeDoubleAccessor (&LrWpanRadioEnergyModel::SetTrxSwitchingCurrentA,
                                       &LrWpanRadioEnergyModel::GetTrxSwitchingCurrentA),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TrxStartCurrentA",
                   "The default radio Transceiver swiching from off to Tx/Rx current in Ampere.",
                   DoubleValue (0.0005),
                   MakeDoubleAccessor (&LrWpanRadioEnergyModel::SetTrxStartCurrentA,
                                       &LrWpanRadioEnergyModel::GetTrxStartCurrentA),
                   MakeDoubleChecker<double> ())

    .AddTraceSource ("TotalEnergyConsumption",
                     "Total energy consumption of the radio device.",
                     MakeTraceSourceAccessor (&LrWpanRadioEnergyModel::m_totalEnergyConsumption),
                     "ns3::TracedValueCallback::Double")
  ; 
  return tid;
}

LrWpanRadioEnergyModel::LrWpanRadioEnergyModel ()
{
  NS_LOG_FUNCTION (this);
  m_lastUpdateTime = Seconds (0.0);
  m_energyDepletionCallback.Nullify ();
  m_source = NULL;
  m_currentState = IEEE_802_15_4_PHY_TRX_OFF;
  m_sourceEnergyUnlimited = 0;
  m_remainingBatteryEnergy = 0;
  m_sourcedepleted = 0;
}

LrWpanRadioEnergyModel::~LrWpanRadioEnergyModel ()
{
  NS_LOG_FUNCTION (this);
}

void
LrWpanRadioEnergyModel::SetEnergySource (Ptr<EnergySource> source)
{
  NS_LOG_FUNCTION (this << source);
  NS_ASSERT (source != NULL);
  m_source = source;
  m_energyToDecrease = 0;
  m_remainingBatteryEnergy = m_source->GetInitialEnergy();
  m_sourceEnergyUnlimited = DynamicCast<LrWpanEnergySource> (m_source)->GetEnergyUnlimited ();
}

double
LrWpanRadioEnergyModel::GetTotalEnergyConsumption (void) const
{
  NS_LOG_FUNCTION (this);
  return m_totalEnergyConsumption;
}

double
LrWpanRadioEnergyModel::GetTrxOffCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_TrxOffCurrentA;
}

void
LrWpanRadioEnergyModel::SetTrxOffCurrentA (double TrxOffCurrentA)
{
  NS_LOG_FUNCTION (this << TrxOffCurrentA);
  m_TrxOffCurrentA = TrxOffCurrentA;
}

double
LrWpanRadioEnergyModel::GetRxBusyCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_RxBusyCurrentA;
}

void
LrWpanRadioEnergyModel::SetRxBusyCurrentA (double RxBusyCurrentA)
{
  NS_LOG_FUNCTION (this << RxBusyCurrentA);
  m_RxBusyCurrentA = RxBusyCurrentA;
}

double
LrWpanRadioEnergyModel::GetTxCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_TxCurrentA;
}

void
LrWpanRadioEnergyModel::SetTxCurrentA (double txCurrentA)
{
  NS_LOG_FUNCTION (this << txCurrentA);
  m_TxCurrentA = txCurrentA;
}

double
LrWpanRadioEnergyModel::GetRxCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_RxCurrentA;
}

void
LrWpanRadioEnergyModel::SetRxCurrentA (double rxCurrentA)
{
  NS_LOG_FUNCTION (this << rxCurrentA);
  m_RxCurrentA = rxCurrentA;
}

double
LrWpanRadioEnergyModel::GetTxBusyCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_TxBusyCurrentA;
}

void
LrWpanRadioEnergyModel::SetTxBusyCurrentA (double TxBusyCurrentA)
{
  NS_LOG_FUNCTION (this << TxBusyCurrentA);
  m_TxBusyCurrentA = TxBusyCurrentA;
}

double
LrWpanRadioEnergyModel::GetTrxSwitchingCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_TrxSwitchingCurrentA;
}

void
LrWpanRadioEnergyModel::SetTrxSwitchingCurrentA (double TrxSwitchingCurrentA)
{
  NS_LOG_FUNCTION (this << TrxSwitchingCurrentA);
  m_TrxSwitchingCurrentA = TrxSwitchingCurrentA;
}

double
LrWpanRadioEnergyModel::GetTrxStartCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_TrxStartCurrentA;
}

void
LrWpanRadioEnergyModel::SetTrxStartCurrentA (double TrxStartCurrentA)
{
  NS_LOG_FUNCTION (this << TrxStartCurrentA);
  m_TrxStartCurrentA = TrxStartCurrentA;
}

LrWpanPhyEnumeration
LrWpanRadioEnergyModel::GetCurrentState (void) const
{
  NS_LOG_FUNCTION (this);
  return m_currentState;
}

void
LrWpanRadioEnergyModel::SetEnergyDepletionCallback (
  LrWpanRadioEnergyDepletionCallback callback)
{
  NS_LOG_FUNCTION (this);
  if (callback.IsNull ())
    {
      NS_LOG_DEBUG ("LrWpanRadioEnergyModel:setting NULL energy depletion callback!");
    }
  m_energyDepletionCallback = callback;
}

void
LrWpanRadioEnergyModel::ChangeLrWpanState (Time t, LrWpanPhyEnumeration oldstate, LrWpanPhyEnumeration newstate)
{
  NS_LOG_FUNCTION (this << newstate);

  Time duration = Simulator::Now () - m_lastUpdateTime;
  NS_ASSERT (duration.GetNanoSeconds () >= 0); // check if duration is valid

  // energy to decrease = current * voltage * time
  if (newstate != IEEE_802_15_4_PHY_FORCE_TRX_OFF)
    {
      m_energyToDecrease = 0.0;
      double supplyVoltage = m_source->GetSupplyVoltage ();

      switch (m_currentState)
        {
        case IEEE_802_15_4_PHY_TRX_OFF:
          m_energyToDecrease = duration.GetSeconds () * m_TrxOffCurrentA * supplyVoltage;
          break;
        case IEEE_802_15_4_PHY_BUSY_RX:
          m_energyToDecrease = duration.GetSeconds () * m_RxBusyCurrentA * supplyVoltage;
          break;
        case IEEE_802_15_4_PHY_TX_ON:
          m_energyToDecrease = duration.GetSeconds () * m_TxCurrentA * supplyVoltage;
          break;
        case IEEE_802_15_4_PHY_RX_ON:
          m_energyToDecrease = duration.GetSeconds () * m_RxCurrentA * supplyVoltage;
          break;
        case IEEE_802_15_4_PHY_BUSY_TX:
          m_energyToDecrease = duration.GetSeconds () * m_TxBusyCurrentA * supplyVoltage;
          break;
        case IEEE_802_15_4_PHY_TRX_SWITCHING:
          m_energyToDecrease = duration.GetSeconds () * m_TrxSwitchingCurrentA * supplyVoltage;
          break;
        case IEEE_802_15_4_PHY_TRX_START:
          m_energyToDecrease = duration.GetSeconds () * m_TrxStartCurrentA * supplyVoltage;
          break;
        case IEEE_802_15_4_PHY_FORCE_TRX_OFF:
          m_energyToDecrease = 0;
          break;
        default:
          NS_FATAL_ERROR ("LrWpanRadioEnergyModel:Undefined radio state: " << m_currentState);
        }

      // update total energy consumption
      m_totalEnergyConsumption += m_energyToDecrease;

      // update last update time stamp
      m_lastUpdateTime = Simulator::Now ();

      // notify energy source
      m_source->UpdateEnergySource ();
    }

  if (!m_sourcedepleted)
    {
      SetLrWpanRadioState (newstate);
      NS_LOG_DEBUG ("LrWpanRadioEnergyModel:Total energy consumption is " <<
                    m_totalEnergyConsumption << "J");
    }
}

 void
 LrWpanRadioEnergyModel::ChangeState (int newState)
{
}

void
LrWpanRadioEnergyModel::HandleEnergyChanged (void)
{
  //NS_LOG_FUNCTION (this);
  //NS_LOG_DEBUG ("LoraRadioEnergyModel:Energy changed!");
}

void
LrWpanRadioEnergyModel::HandleEnergyDepletion (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("LrWpanRadioEnergyModel:Energy is depleted!");
  // invoke energy depletion callback, if set.
  if (!m_energyDepletionCallback.IsNull ())
    {
      m_energyDepletionCallback ();
    }
  m_sourcedepleted = 1;
}

/*
 * Private functions start here.
 */

void
LrWpanRadioEnergyModel::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_source = NULL;
  m_energyDepletionCallback.Nullify ();
}


double
LrWpanRadioEnergyModel::DoGetCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  switch (m_currentState)
    {
    case IEEE_802_15_4_PHY_TRX_OFF:
      return m_TrxOffCurrentA;
    case IEEE_802_15_4_PHY_BUSY_RX:
      return m_RxBusyCurrentA;
    case IEEE_802_15_4_PHY_TX_ON:
      return m_TxCurrentA;
    case IEEE_802_15_4_PHY_RX_ON:
      return m_RxCurrentA;
    case IEEE_802_15_4_PHY_BUSY_TX:
      return m_TxBusyCurrentA;
    case IEEE_802_15_4_PHY_TRX_SWITCHING:
      return m_TrxSwitchingCurrentA;
    case IEEE_802_15_4_PHY_TRX_START:
      return m_TrxStartCurrentA;
    case IEEE_802_15_4_PHY_FORCE_TRX_OFF:
      return 0;
    default:
      NS_FATAL_ERROR ("LrWpanRadioEnergyModel:Undefined radio state:" << m_currentState);
    }
}


void
LrWpanRadioEnergyModel::SetLrWpanRadioState (const LrWpanPhyEnumeration state)
{
  NS_LOG_FUNCTION (this << state);

  std::string preStateName;
  switch (m_currentState)
    {
    case IEEE_802_15_4_PHY_TRX_OFF:
      preStateName = "TRX_OFF";
      break;
    case IEEE_802_15_4_PHY_BUSY_RX:
      preStateName = "RX_BUSY";
      break;
    case IEEE_802_15_4_PHY_TX_ON:
      preStateName = "TX";
      break;
    case IEEE_802_15_4_PHY_RX_ON:
      preStateName = "RX";
      break;
    case IEEE_802_15_4_PHY_BUSY_TX:
      preStateName = "TX_BUSY";
      break;
    case IEEE_802_15_4_PHY_TRX_SWITCHING:
      preStateName = "TRX_SWITCH";
      break;
    case IEEE_802_15_4_PHY_TRX_START:
      preStateName = "TRX_START";
      break;
    case IEEE_802_15_4_PHY_FORCE_TRX_OFF:
      preStateName = "TRX_FORCE_OFF";
      break;
  default:
    NS_FATAL_ERROR ("LrWpanRadioEnergyModel:Undefined radio state: " << m_currentState);
  }

  m_currentState = state;
  std::string curStateName;
  switch (state)
    {
    case IEEE_802_15_4_PHY_TRX_OFF:
      curStateName = "TRX_OFF";
      break;
    case IEEE_802_15_4_PHY_BUSY_RX:
      curStateName = "RX_BUSY";
      break;
    case IEEE_802_15_4_PHY_TX_ON:
      curStateName = "TX";
      break;
    case IEEE_802_15_4_PHY_RX_ON:
      curStateName = "RX";
      break;
    case IEEE_802_15_4_PHY_BUSY_TX:
      curStateName = "TX_BUSY";
      break;
    case IEEE_802_15_4_PHY_TRX_SWITCHING:
      curStateName = "TRX_SWITCH";
      break;
    case IEEE_802_15_4_PHY_TRX_START:
      curStateName = "TRX_START";
      break;
    case IEEE_802_15_4_PHY_FORCE_TRX_OFF:
      curStateName = "TRX_FORCE_OFF";
      break;
  default:
    NS_FATAL_ERROR ("LrWpanRadioEnergyModel:Undefined radio state: " << m_currentState);
  }

  m_remainingBatteryEnergy = m_source -> GetRemainingEnergy();

  m_EnergyStateLogger (preStateName, curStateName, m_sourceEnergyUnlimited, m_energyToDecrease, m_remainingBatteryEnergy, m_totalEnergyConsumption);

  NS_LOG_DEBUG ("LrWpanRadioEnergyModel:Switching to state: " << curStateName <<
                " at time = " << Simulator::Now ());
}


// -------------------------------------------------------------------------- //


/*
 * Private function state here.
 */



} // namespace ns3