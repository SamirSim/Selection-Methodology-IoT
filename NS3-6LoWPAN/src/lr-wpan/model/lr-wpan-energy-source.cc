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
 * Authors: Sidharth Nabar <snabar@uw.edu>, He Wu <mdzz@u.washington.edu>, Peishuo Li <pressthunder@gmail.com>
 */

#include "lr-wpan-energy-source.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/double.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/simulator.h"
#include "ns3/boolean.h"

NS_LOG_COMPONENT_DEFINE ("LrWpanEnergySource");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (LrWpanEnergySource);

TypeId
LrWpanEnergySource::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LrWpanEnergySource")
    .SetParent<EnergySource> ()
    .AddConstructor<LrWpanEnergySource> ()
    .AddAttribute ("LrWpanEnergySourceInitialEnergyJ",
                   "Initial energy stored in basic energy source.",
                   DoubleValue (10),  // in Joules
                   MakeDoubleAccessor (&LrWpanEnergySource::SetInitialEnergy,
                                       &LrWpanEnergySource::GetInitialEnergy),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("LrWpanEnergySupplyVoltageV",
                   "Initial supply voltage for basic energy source.",
                   DoubleValue (3.0), // in Volts
                   MakeDoubleAccessor (&LrWpanEnergySource::SetSupplyVoltage,
                                       &LrWpanEnergySource::GetSupplyVoltage),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("PeriodicEnergyUpdateInterval",
                   "Time between two consecutive periodic energy updates.",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&LrWpanEnergySource::SetEnergyUpdateInterval,
                                     &LrWpanEnergySource::GetEnergyUpdateInterval),
                   MakeTimeChecker ())
    .AddAttribute ("LrWpanUnlimitedEnergy",
                   "Energy unlimited or not at LrWpanEnergySource.",
                    BooleanValue (true),
                    MakeBooleanAccessor (&LrWpanEnergySource::SetEnergyUnlimited,
                                         &LrWpanEnergySource::GetEnergyUnlimited),
                    MakeBooleanChecker ())
    .AddTraceSource ("RemainingEnergy",
                     "Remaining energy at LrWpanEnergySource.",
                     MakeTraceSourceAccessor (&LrWpanEnergySource::m_remainingEnergyJ),
                     "ns3::LrWpanEnergySource::SentTracedCallBack")

  ;
  return tid;
}

LrWpanEnergySource::LrWpanEnergySource ()
{
  NS_LOG_FUNCTION (this);
  m_lastUpdateTime = Seconds (0.0);
}

LrWpanEnergySource::~LrWpanEnergySource ()
{
  NS_LOG_FUNCTION (this);
}

void
LrWpanEnergySource::SetInitialEnergy (double initialEnergyJ)
{
  NS_LOG_FUNCTION (this << initialEnergyJ);
  NS_ASSERT (initialEnergyJ >= 0);
  m_initialEnergyJ = initialEnergyJ;
  m_remainingEnergyJ = m_initialEnergyJ;
}

void
LrWpanEnergySource::SetSupplyVoltage (double supplyVoltageV)
{
  NS_LOG_FUNCTION (this << supplyVoltageV);
  m_supplyVoltageV = supplyVoltageV;
}

void
LrWpanEnergySource::SetEnergyUpdateInterval (Time interval)
{
  NS_LOG_FUNCTION (this << interval);
  m_energyUpdateInterval = interval;
}

Time
LrWpanEnergySource::GetEnergyUpdateInterval (void) const
{
  NS_LOG_FUNCTION (this);
  return m_energyUpdateInterval;
}

double
LrWpanEnergySource::GetSupplyVoltage (void) const
{
  NS_LOG_FUNCTION (this);
  return m_supplyVoltageV;
}

double
LrWpanEnergySource::GetInitialEnergy (void) const
{
  NS_LOG_FUNCTION (this);
  return m_initialEnergyJ;
}

double
LrWpanEnergySource::GetRemainingEnergy (void)
{
  NS_LOG_FUNCTION (this);

  NS_LOG_DEBUG ("Get RemainingEnergy");
  // update energy source to get the latest remaining energy.
  //UpdateEnergySource ();
  return m_remainingEnergyJ;
}

double
LrWpanEnergySource::GetEnergyFraction (void)
{
  NS_LOG_FUNCTION (this);
  // update energy source to get the latest remaining energy.
  UpdateEnergySource ();
  return m_remainingEnergyJ / m_initialEnergyJ;
}

void
LrWpanEnergySource::UpdateEnergySource (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("LrWpanEnergySource:Updating remaining energy.");

  // do not update if simulation has finished
  if (Simulator::IsFinished ())
    {
      return;
    }

  m_energyUpdateEvent.Cancel ();

  CalculateRemainingEnergy ();

  m_lastUpdateTime = Simulator::Now ();

  if (m_remainingEnergyJ <= 0)
    {
      HandleEnergyDrainedEvent ();
      return; // stop periodic update
    }

  m_energyUpdateEvent = Simulator::Schedule (m_energyUpdateInterval,
                                             &LrWpanEnergySource::UpdateEnergySource,
                                             this);
}

/*
 * Private functions start here.
 */

void
LrWpanEnergySource::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  UpdateEnergySource ();  // start periodic update
}

void
LrWpanEnergySource::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  EnergySource::BreakDeviceEnergyModelRefCycle ();  // break reference cycle
}

void
LrWpanEnergySource::HandleEnergyDrainedEvent (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("LrWpanEnergySource:Energy depleted!");
  m_remainingEnergyJ = 0; // energy never goes below 0
  NotifyEnergyDrained (); // notify DeviceEnergyModel objects
  NS_LOG_DEBUG ("LrWpanEnergySource:Remaining energy = " << m_remainingEnergyJ);
}

void
LrWpanEnergySource::CalculateRemainingEnergy (void)
{
  NS_LOG_FUNCTION (this);
  double totalCurrentA = CalculateTotalCurrent ();
  Time duration = Simulator::Now () - m_lastUpdateTime;
  NS_ASSERT (duration.GetSeconds () >= 0);
  // energy = current * voltage * time

  if (!m_unlimited)
  {
    double energyToDecreaseJ = totalCurrentA * m_supplyVoltageV * duration.GetSeconds ();
    m_remainingEnergyJ -= energyToDecreaseJ;
    NS_LOG_DEBUG ("LrWpanEnergySource:Remaining energy = " << m_remainingEnergyJ);
  }
  else
    NS_LOG_DEBUG ("LrWpanEnergySource Energy Unlimited");
}

void
LrWpanEnergySource::SetEnergyUnlimited (bool unlimit)
{
  NS_LOG_FUNCTION (this);
  m_unlimited = unlimit;
}

bool
LrWpanEnergySource::GetEnergyUnlimited (void) const
{
    NS_LOG_FUNCTION (this);
    return m_unlimited;
}

} // namespace ns3