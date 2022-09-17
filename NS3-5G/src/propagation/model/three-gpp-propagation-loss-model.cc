/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 SIGNET Lab, Department of Information Engineering,
 * University of Padova
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

#include "three-gpp-propagation-loss-model.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/channel-condition-model.h"
#include "ns3/okumura-hata-propagation-loss-model.h"
#include "ns3/itu-r-1411-los-propagation-loss-model.h"
#include "ns3/itu-r-1238-propagation-loss-model.h"
#include "ns3/itu-r-1411-nlos-over-rooftop-propagation-loss-model.h"
#include "ns3/hybrid-buildings-propagation-loss-model.h"
#include "ns3/itu-r-1238-propagation-loss-model.h"
#include "ns3/kun-2600-mhz-propagation-loss-model.h"
#include "ns3/double.h"
#include "ns3/boolean.h"
#include "ns3/pointer.h"
#include <cmath>
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/enum.h"
#include "ns3/building.h"
#include "ns3/mobility-building-info.h"
#include "ns3/building-list.h"
#include "ns3/buildings-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ThreeGppPropagationLossModel");

static const double M_C = 3.0e8; // propagation velocity in free space

// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED (ThreeGppPropagationLossModel);

TypeId
ThreeGppPropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ThreeGppPropagationLossModel")
    .SetParent<PropagationLossModel> ()
    .SetGroupName ("Propagation")
    .AddAttribute ("ChannelConditionModel", "Pointer to the channel condition model.",
                   PointerValue (),
                   MakePointerAccessor (&ThreeGppPropagationLossModel::SetChannelConditionModel,
                                        &ThreeGppPropagationLossModel::GetChannelConditionModel),
                   MakePointerChecker<ChannelConditionModel> ())
  ;
  return tid;
}

ThreeGppPropagationLossModel::ThreeGppPropagationLossModel ()
  : PropagationLossModel ()
{
  NS_LOG_FUNCTION (this);

  // initialize the normal random variable
  m_normRandomVariable = CreateObject<NormalRandomVariable> ();
  m_normRandomVariable->SetAttribute ("Mean", DoubleValue (0));
  m_normRandomVariable->SetAttribute ("Variance", DoubleValue (1));
}

ThreeGppPropagationLossModel::~ThreeGppPropagationLossModel ()
{
  NS_LOG_FUNCTION (this);
}

void
ThreeGppPropagationLossModel::DoDispose ()
{
  m_channelConditionModel->Dispose ();
  m_channelConditionModel = nullptr;
  m_shadowingMap.clear ();
}

void
ThreeGppPropagationLossModel::SetChannelConditionModel (Ptr<ChannelConditionModel> model)
{
  NS_LOG_FUNCTION (this);
  m_channelConditionModel = model;
}

Ptr<ChannelConditionModel>
ThreeGppPropagationLossModel::GetChannelConditionModel () const
{
  NS_LOG_FUNCTION (this);
  return m_channelConditionModel;
}

void
ThreeGppPropagationLossModel::SetFrequency (double f)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (f >= 500.0e6 && f <= 100.0e9, "Frequency should be between 0.5 and 100 GHz but is " << f);
  m_frequency = f;
}

double
ThreeGppPropagationLossModel::GetFrequency () const
{
  NS_LOG_FUNCTION (this);
  return m_frequency;
}

double
ThreeGppPropagationLossModel::DoCalcRxPower (double txPowerDbm,
                                             Ptr<MobilityModel> a,
                                             Ptr<MobilityModel> b) const
{

  Ptr<ChannelCondition> cond = m_channelConditionModel->GetChannelCondition (a, b);

  // compute the 2D distance between a and b
  double distance2d = Calculate2dDistance (a->GetPosition (), b->GetPosition ());

  // compute the 3D distance between a and b
  double distance3d = CalculateDistance (a->GetPosition (), b->GetPosition ());

  // compute hUT and hBS
  std::pair<double, double> heights = GetUtAndBsHeights (a->GetPosition ().z, b->GetPosition ().z);
  NS_LOG_DEBUG(a << " " << b);
  double rxPow = txPowerDbm;
  rxPow -= GetLoss (cond, distance2d, distance3d, heights.first, heights.second, a, b); 

  return txPowerDbm + rxPow;
}

double
ThreeGppPropagationLossModel::GetLoss (Ptr<ChannelCondition> cond, double distance2d, double distance3d, double hUt, double hBs, Ptr<MobilityModel> a,
                                             Ptr<MobilityModel> b) const
{
  NS_LOG_FUNCTION (this);
  cond->SetLosCondition(ChannelCondition::LosConditionValue::LOS);
  double loss = 0;
  if (cond->GetLosCondition () == ChannelCondition::LosConditionValue::LOS)
    {
      loss = GetLossLos (distance2d, distance3d, hUt, hBs, a, b);
    }
  else if (cond->GetLosCondition () == ChannelCondition::LosConditionValue::NLOSv)
    {
      loss = GetLossNlosv (distance2d, distance3d, hUt, hBs);
    }
  else if (cond->GetLosCondition () == ChannelCondition::LosConditionValue::NLOS)
    {
      loss = GetLossNlos (distance2d, distance3d, hUt, hBs);
    }
  else
    {
      NS_FATAL_ERROR ("Unknown channel condition");
    }
  return loss;
}

double
ThreeGppPropagationLossModel::GetLossNlosv (double distance2D, double distance3D, double hUt, double hBs) const
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("Unsupported channel condition (NLOSv)");
  return 0;
}

double
ThreeGppPropagationLossModel::GetShadowing (Ptr<MobilityModel> a, Ptr<MobilityModel> b, ChannelCondition::LosConditionValue cond) const
{
  NS_LOG_FUNCTION (this);

  double shadowingValue;

  // compute the channel key
  uint32_t key = GetKey (a, b);

  bool notFound = false; // indicates if the shadowing value has not been computed yet
  bool newCondition = false; // indicates if the channel condition has changed
  Vector newDistance; // the distance vector, that is not a distance but a difference
  auto it = m_shadowingMap.end (); // the shadowing map iterator
  if (m_shadowingMap.find (key) != m_shadowingMap.end ())
    {
      // found the shadowing value in the map
      it = m_shadowingMap.find (key);
      newDistance = GetVectorDifference (a, b);
      newCondition = (it->second.m_condition != cond); // true if the condition changed
    }
  else
    {
      notFound = true;

      // add a new entry in the map and update the iterator
      ShadowingMapItem newItem;
      it = m_shadowingMap.insert (it, std::make_pair (key, newItem));
    }

  if (notFound || newCondition)
    {
      // generate a new independent realization
      shadowingValue = m_normRandomVariable->GetValue () * GetShadowingStd (a, b, cond);
    }
  else
    {
      // compute a new correlated shadowing loss
      Vector2D displacement (newDistance.x - it->second.m_distance.x, newDistance.y - it->second.m_distance.y);
      double R = exp (-1 * displacement.GetLength () / GetShadowingCorrelationDistance (cond));
      shadowingValue =  R * it->second.m_shadowing + sqrt (1 - R * R) * m_normRandomVariable->GetValue () * GetShadowingStd (a, b, cond);
    }

  // update the entry in the map
  it->second.m_shadowing = shadowingValue;
  it->second.m_distance = newDistance; // Save the (0,0,0) vector in case it's the first time we are calculating this value
  it->second.m_condition = cond;

  return shadowingValue;
}

std::pair<double, double>
ThreeGppPropagationLossModel::GetUtAndBsHeights (double za, double zb) const
{
  // The default implementation assumes that the tallest node is the BS and the
  // smallest is the UT.
  double hUt = std::min (za, zb);
  double hBs = std::max (za, zb);

  return std::pair<double, double> (hUt, hBs);
}

int64_t
ThreeGppPropagationLossModel::DoAssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this);

  m_normRandomVariable->SetStream (stream);
  return 1;
}

double
ThreeGppPropagationLossModel::Calculate2dDistance (Vector a, Vector b)
{
  double x = a.x - b.x;
  double y = a.y - b.y;
  double distance2D = sqrt (x * x + y * y);

  return distance2D;
}

uint32_t
ThreeGppPropagationLossModel::GetKey (Ptr<MobilityModel> a, Ptr<MobilityModel> b)
{
  // use the nodes ids to obtain an unique key for the channel between a and b
  // sort the nodes ids so that the key is reciprocal
  uint32_t x1 = std::min (a->GetObject<Node> ()->GetId (), b->GetObject<Node> ()->GetId ());
  uint32_t x2 = std::max (a->GetObject<Node> ()->GetId (), b->GetObject<Node> ()->GetId ());

  // use the cantor function to obtain the key
  uint32_t key = (((x1 + x2) * (x1 + x2 + 1)) / 2) + x2;

  return key;
}

Vector
ThreeGppPropagationLossModel::GetVectorDifference (Ptr<MobilityModel> a, Ptr<MobilityModel> b)
{
  uint32_t x1 = a->GetObject<Node> ()->GetId ();
  uint32_t x2 = b->GetObject<Node> ()->GetId ();

  if (x1 < x2)
    {
      return b->GetPosition () - a->GetPosition ();
    }
  else
    {
      return a->GetPosition () - b->GetPosition ();
    }
}

// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED (ThreeGppRmaPropagationLossModel); // Log-Distance

TypeId
ThreeGppRmaPropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ThreeGppRmaPropagationLossModel")
    .SetParent<ThreeGppPropagationLossModel> ()
    .SetGroupName ("Propagation")
    .AddConstructor<ThreeGppRmaPropagationLossModel> ()
    .AddAttribute ("AvgBuildingHeight", "The average building height in meters.",
                   DoubleValue (5.0),
                   MakeDoubleAccessor (&ThreeGppRmaPropagationLossModel::m_h),
                   MakeDoubleChecker<double> (5.0, 50.0))
    .AddAttribute ("AvgStreetWidth", "The average street width in meters.",
                   DoubleValue (20.0),
                   MakeDoubleAccessor (&ThreeGppRmaPropagationLossModel::m_w),
                   MakeDoubleChecker<double> (5.0, 50.0))
    .AddAttribute ("Exponent",
                   "The exponent of the Path Loss propagation model",
                   DoubleValue (3.0),
                   MakeDoubleAccessor (&ThreeGppRmaPropagationLossModel::m_exponent),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("ReferenceDistance",
                   "The distance at which the reference loss is calculated (m)",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&ThreeGppRmaPropagationLossModel::m_referenceDistance),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("ReferenceLoss",
                   "The reference loss at reference distance (dB). (Default is Friis at 1m with 5.15 GHz)",
                   DoubleValue (46.6777),
                   MakeDoubleAccessor (&ThreeGppRmaPropagationLossModel::m_referenceLoss),
                   MakeDoubleChecker<double> ()) 
  ;
  return tid;
}

ThreeGppRmaPropagationLossModel::ThreeGppRmaPropagationLossModel ()
  : ThreeGppPropagationLossModel ()
{
  NS_LOG_FUNCTION (this);

  // set a default channel condition model
  m_channelConditionModel = CreateObject<ThreeGppRmaChannelConditionModel> ();
}

ThreeGppRmaPropagationLossModel::~ThreeGppRmaPropagationLossModel () // LogDistance
{
  NS_LOG_FUNCTION (this);
}

double
ThreeGppRmaPropagationLossModel::GetLossLos (double distance2D, double distance3D, double hUt, double hBs, Ptr<MobilityModel> a,
                                             Ptr<MobilityModel> b) const
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("RMA");

  if (distance2D <= m_referenceDistance)
    {
      return m_referenceLoss;
    }

  double pathLossDb = 10 * m_exponent * std::log10 (distance2D / m_referenceDistance);
  double rxc = -m_referenceLoss - pathLossDb;
  NS_LOG_DEBUG ("distance="<<distance2D<<"m, reference-attenuation="<< -m_referenceLoss<<"dB, "<<
                "attenuation coefficient="<<rxc<<"db");

  return rxc;
}

double
ThreeGppRmaPropagationLossModel::GetLossNlos (double distance2D, double distance3D, double hUt, double hBs) const
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (m_frequency <= 30.0e9, "RMa scenario is valid for frequencies between 0.5 and 30 GHz.");

  // check if hBs and hUt are within the validity range
  if (hUt < 1.0 || hUt > 10.0)
    {
      NS_LOG_WARN ("The height of the UT should be between 1 and 10 m (see TR 38.901, Table 7.4.1-1)");
    }

  if (hBs < 10.0 || hBs > 150.0)
    {
      NS_LOG_WARN ("The height of the BS should be between 10 and 150 m (see TR 38.901, Table 7.4.1-1)");
    }

  // NOTE The model is intended to be used for BS-UT links, however we may need to
  // compute the pathloss between two BSs or UTs, e.g., to evaluate the
  // interference. In order to apply the model, we need to retrieve the values of
  // hBS and hUT, but in these cases one of the two falls outside the validity
  // range and the warning message is printed (hBS for the UT-UT case and hUT
  // for the BS-BS case).

  // check if the distance is outside the validity range
  if (distance2D < 10.0 || distance2D > 5.0e3)
    {
      NS_LOG_WARN ("The 2D distance is outside the validity range, the pathloss value may not be accurate");
    }

  // compute the pathloss
  double plNlos = 161.04 - 7.1 * log10 (m_w) + 7.5 * log10 (m_h) - (24.37 - 3.7 * pow ((m_h / hBs), 2)) * log10 (hBs) + (43.42 - 3.1 * log10 (hBs)) * (log10 (distance3D) - 3.0) + 20.0 * log10 (m_frequency / 1e9) - (3.2 * pow (log10 (11.75 * hUt), 2) - 4.97);
  Ptr<MobilityModel> a;
  Ptr<MobilityModel> b;
  double loss = std::max (GetLossLos (distance2D, distance3D, hUt, hBs, a, b), plNlos);

  NS_LOG_DEBUG ("Loss " << loss);

  return loss;
}

double
ThreeGppRmaPropagationLossModel::GetShadowingStd (Ptr<MobilityModel> a, Ptr<MobilityModel> b, ChannelCondition::LosConditionValue cond) const
{
  NS_LOG_FUNCTION (this);
  double shadowingStd;

  if (cond == ChannelCondition::LosConditionValue::LOS)
    {
      // compute the 2D distance between the two nodes
      double distance2d = Calculate2dDistance (a->GetPosition (), b->GetPosition ());

      // compute the breakpoint distance (see 3GPP TR 38.901, Table 7.4.1-1, note 5)
      double distanceBp = GetBpDistance (m_frequency, a->GetPosition ().z, b->GetPosition ().z);

      if (distance2d <= distanceBp)
        {
          shadowingStd = 4.0;
        }
      else
        {
          shadowingStd = 6.0;
        }
    }
  else if (cond == ChannelCondition::LosConditionValue::NLOS)
    {
      shadowingStd = 8.0;
    }
  else
    {
      NS_FATAL_ERROR ("Unknown channel condition");
    }

  return shadowingStd;
}

double
ThreeGppRmaPropagationLossModel::GetShadowingCorrelationDistance (ChannelCondition::LosConditionValue cond) const
{
  NS_LOG_FUNCTION (this);
  double correlationDistance;

  // See 3GPP TR 38.901, Table 7.5-6
  if (cond == ChannelCondition::LosConditionValue::LOS)
    {
      correlationDistance = 37;
    }
  else if (cond == ChannelCondition::LosConditionValue::NLOS)
    {
      correlationDistance = 120;
    }
  else
    {
      NS_FATAL_ERROR ("Unknown channel condition");
    }

  return correlationDistance;
}

double
ThreeGppRmaPropagationLossModel::Pl1 (double frequency, double distance3D, double h, double w)
{
  NS_UNUSED (w);
  double loss = 20.0 * log10 (40.0 * M_PI * distance3D * frequency / 1e9 / 3.0) + std::min (0.03 * pow (h, 1.72), 10.0) * log10 (distance3D) - std::min (0.044 * pow (h, 1.72), 14.77) + 0.002 * log10 (h) * distance3D;
  return loss;
}

double
ThreeGppRmaPropagationLossModel::GetBpDistance (double frequency, double hA, double hB)
{
  double distanceBp = 2.0 * M_PI * hA * hB * frequency / M_C;
  return distanceBp;
}

// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED (ThreeGppUmaPropagationLossModel); // COST-HATA

TypeId
ThreeGppUmaPropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ThreeGppUmaPropagationLossModel")
    .SetParent<ThreeGppPropagationLossModel> ()
    .SetGroupName ("Propagation")
    .AddConstructor<ThreeGppUmaPropagationLossModel> ()
    .AddAttribute ("Lambda",
                   "The wavelength  (default is 2.3 GHz at 300 000 km/s).",
                   DoubleValue (300000000.0 / 28e9),
                   MakeDoubleAccessor (&ThreeGppUmaPropagationLossModel::m_lambda),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Frequency",
                   "The Frequency  (default is 2.3 GHz).",
                   DoubleValue (28e9),
                   MakeDoubleAccessor (&ThreeGppUmaPropagationLossModel::m_frequency),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("BSAntennaHeight",
                   "BS Antenna Height (default is 50m).",
                   DoubleValue (50.0),
                   MakeDoubleAccessor (&ThreeGppUmaPropagationLossModel::m_BSAntennaHeight),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("SSAntennaHeight",
                   "SS Antenna Height (default is 3m).",
                   DoubleValue (3),
                   MakeDoubleAccessor (&ThreeGppUmaPropagationLossModel::m_SSAntennaHeight),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("MinDistance",
                   "The distance under which the propagation model refuses to give results (m) ",
                   DoubleValue (0.5),
                   MakeDoubleAccessor (&ThreeGppUmaPropagationLossModel::SetMinDistance, &ThreeGppUmaPropagationLossModel::GetMinDistance),
                   MakeDoubleChecker<double> ());
  ;
  return tid;
}

ThreeGppUmaPropagationLossModel::ThreeGppUmaPropagationLossModel ()
  : ThreeGppPropagationLossModel ()
{
  NS_LOG_FUNCTION (this);
  m_uniformVar = CreateObject<UniformRandomVariable> ();

  // set a default channel condition model
  m_channelConditionModel = CreateObject<ThreeGppUmaChannelConditionModel> ();
}

ThreeGppUmaPropagationLossModel::~ThreeGppUmaPropagationLossModel ()
{
  NS_LOG_FUNCTION (this);
}

void
ThreeGppUmaPropagationLossModel::SetMinDistance (double minDistance)
{
  m_minDistance = minDistance;
}
double
ThreeGppUmaPropagationLossModel::GetMinDistance (void) const
{
  return m_minDistance;
}

double
ThreeGppUmaPropagationLossModel::GetBpDistance (double hUt, double hBs, double distance2D) const
{
  NS_LOG_FUNCTION (this);

  // compute g (d2D) (see 3GPP TR 38.901, Table 7.4.1-1, Note 1)
  double g = 0.0;
  if (distance2D > 18.0)
    {
      g = 5.0 / 4.0 * pow (distance2D / 100.0, 3) * exp (-distance2D / 150.0);
    }

  // compute C (hUt, d2D) (see 3GPP TR 38.901, Table 7.4.1-1, Note 1)
  double c = 0.0;
  if (hUt >= 13.0)
    {
      c = pow ((hUt - 13.0) / 10.0, 1.5) * g;
    }

  // compute hE (see 3GPP TR 38.901, Table 7.4.1-1, Note 1)
  double prob = 1.0 / (1.0 + c);
  double hE = 0.0;
  if (m_uniformVar->GetValue () < prob)
    {
      hE = 1.0;
    }
  else
    {
      int random = m_uniformVar->GetInteger (12, (int)(hUt - 1.5));
      hE = (double)floor (random / 3.0) * 3.0;
    }

  // compute dBP' (see 3GPP TR 38.901, Table 7.4.1-1, Note 1)
  double distanceBp = 4 * (hBs - hE) * (hUt - hE) * m_frequency / M_C;

  return distanceBp;
}

double
ThreeGppUmaPropagationLossModel::GetLossLos (double distance2D, double distance3D, double hUt, double hBs, Ptr<MobilityModel> a,
                                             Ptr<MobilityModel> b) const
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("UMA");

  //std::cout << "Here frequency: " << m_frequency << std::endl;

  if (distance2D <= m_minDistance)
    {
      return 0.0;
    }

  double frequency_MHz = m_frequency * 1e-6;

  double distance_km = distance2D * 1e-3;

  double C_H = 0.8 + ((1.11 * std::log10(frequency_MHz)) - 0.7) * m_SSAntennaHeight - (1.56 * std::log10(frequency_MHz));

  // from the COST231 wiki entry
  // See also http://www.lx.it.pt/cost231/final_report.htm
  // Ch. 4, eq. 4.4.3, pg. 135

  double loss_in_db = 46.3 + (33.9 * std::log10(frequency_MHz)) - (13.82 * std::log10 (m_BSAntennaHeight)) - C_H
              + ((44.9 - 6.55 * std::log10 (m_BSAntennaHeight)) * std::log10 (distance_km)) + m_shadowing;

  NS_LOG_DEBUG ("dist =" << distance2D << ", Path Loss = " << loss_in_db);

  return (0 - loss_in_db);
}

double
ThreeGppUmaPropagationLossModel::GetLossNlos (double distance2D, double distance3D, double hUt, double hBs) const
{
  NS_LOG_FUNCTION (this);

  // check if hBS and hUT are within the vaalidity range
  if (hUt < 1.5 || hUt > 22.5)
    {
      NS_LOG_WARN ("The height of the UT should be between 1.5 and 22.5 m (see TR 38.901, Table 7.4.1-1)");
    }

  if (hBs != 25.0)
    {
      NS_LOG_WARN ("The height of the BS should be equal to 25 m (see TR 38.901, Table 7.4.1-1)");
    }

  // NOTE The model is intended to be used for BS-UT links, however we may need to
  // compute the pathloss between two BSs or UTs, e.g., to evaluate the
  // interference. In order to apply the model, we need to retrieve the values of
  // hBS and hUT, but in these cases one of the two falls outside the validity
  // range and the warning message is printed (hBS for the UT-UT case and hUT
  // for the BS-BS case).

  // check if the distance is outside the validity range
  if (distance2D < 10.0 || distance2D > 5.0e3)
    {
      NS_LOG_WARN ("The 2D distance is outside the validity range, the pathloss value may not be accurate");
    }

  // compute the pathloss
  double plNlos = 13.54 + 39.08 * log10 (distance3D) + 20.0 * log10 (m_frequency / 1e9) - 0.6 * (hUt - 1.5);
  Ptr<MobilityModel> a;
  Ptr<MobilityModel> b;
  double loss = std::max (GetLossLos (distance2D, distance3D, hUt, hBs, a, b), plNlos);
  NS_LOG_DEBUG ("Loss " << loss);

  return loss;
}

double
ThreeGppUmaPropagationLossModel::GetShadowingStd (Ptr<MobilityModel> a, Ptr<MobilityModel> b, ChannelCondition::LosConditionValue cond) const
{
  NS_LOG_FUNCTION (this);
  NS_UNUSED (a);
  NS_UNUSED (b);
  double shadowingStd;

  if (cond == ChannelCondition::LosConditionValue::LOS)
    {
      shadowingStd = 4.0;
    }
  else if (cond == ChannelCondition::LosConditionValue::NLOS)
    {
      shadowingStd = 6.0;
    }
  else
    {
      NS_FATAL_ERROR ("Unknown channel condition");
    }

  return shadowingStd;
}

double
ThreeGppUmaPropagationLossModel::GetShadowingCorrelationDistance (ChannelCondition::LosConditionValue cond) const
{
  NS_LOG_FUNCTION (this);
  double correlationDistance;

  // See 3GPP TR 38.901, Table 7.5-6
  if (cond == ChannelCondition::LosConditionValue::LOS)
    {
      correlationDistance = 37;
    }
  else if (cond == ChannelCondition::LosConditionValue::NLOS)
    {
      correlationDistance = 50;
    }
  else
    {
      NS_FATAL_ERROR ("Unknown channel condition");
    }

  return correlationDistance;
}

int64_t
ThreeGppUmaPropagationLossModel::DoAssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this);

  m_normRandomVariable->SetStream (stream);
  m_uniformVar->SetStream (stream);
  return 2;
}

// ------------------------------------------------------------------------- // Okumura-Hata

NS_OBJECT_ENSURE_REGISTERED (ThreeGppUmiStreetCanyonPropagationLossModel);

TypeId
ThreeGppUmiStreetCanyonPropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ThreeGppUmiStreetCanyonPropagationLossModel")
    .SetParent<ThreeGppPropagationLossModel> ()
    .SetGroupName ("Propagation")
    .AddConstructor<ThreeGppUmiStreetCanyonPropagationLossModel> ()
    .AddAttribute ("Frequency",
                   "The propagation frequency in Hz",
                   DoubleValue (2160e6),
                   MakeDoubleAccessor (&ThreeGppUmiStreetCanyonPropagationLossModel::m_frequency),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Environment",
                   "Environment Scenario",
                   EnumValue (UrbanEnvironment),
                   MakeEnumAccessor (&ThreeGppUmiStreetCanyonPropagationLossModel::m_environment),
                   MakeEnumChecker (UrbanEnvironment, "Urban",
                                    SubUrbanEnvironment, "SubUrban",
                                    OpenAreasEnvironment, "OpenAreas"))
    .AddAttribute ("CitySize",
                   "Dimension of the city",
                   EnumValue (LargeCity),
                   MakeEnumAccessor (&ThreeGppUmiStreetCanyonPropagationLossModel::m_citySize),
                   MakeEnumChecker (SmallCity, "Small",
                                    MediumCity, "Medium",
                                    LargeCity, "Large"));
  ;
  return tid;
}
ThreeGppUmiStreetCanyonPropagationLossModel::ThreeGppUmiStreetCanyonPropagationLossModel ()
  : ThreeGppPropagationLossModel ()
{
  NS_LOG_FUNCTION (this);

  // set a default channel condition model
  m_channelConditionModel = CreateObject<ThreeGppUmiStreetCanyonChannelConditionModel> ();
}

ThreeGppUmiStreetCanyonPropagationLossModel::~ThreeGppUmiStreetCanyonPropagationLossModel ()
{
  NS_LOG_FUNCTION (this);
}

double
ThreeGppUmiStreetCanyonPropagationLossModel::GetBpDistance (double hUt, double hBs, double distance2D) const
{
  NS_LOG_FUNCTION (this);
  NS_UNUSED (distance2D);

  // compute hE (see 3GPP TR 38.901, Table 7.4.1-1, Note 1)
  double hE = 1.0;

  // compute dBP' (see 3GPP TR 38.901, Table 7.4.1-1, Note 1)
  double distanceBp = 4 * (hBs - hE) * (hUt - hE) * m_frequency / M_C;

  return distanceBp;
}

double
ThreeGppUmiStreetCanyonPropagationLossModel::GetLossLos (double distance2D, double distance3D, double hUt, double hBs, Ptr<MobilityModel> a,
                                             Ptr<MobilityModel> b) const
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("StreeCanyon-UMI");

  double loss = 0.0;
  Ptr<OkumuraHataPropagationLossModel> m_okumuraHata = CreateObject<OkumuraHataPropagationLossModel> ();
  NS_LOG_DEBUG("Okumura applied");
  loss = m_okumuraHata->GetLoss (a, b);
  NS_LOG_DEBUG("Okumura applied done " << loss);
  return loss;
}

double
ThreeGppUmiStreetCanyonPropagationLossModel::GetLossNlos (double distance2D, double distance3D, double hUt, double hBs) const
{
  NS_LOG_FUNCTION (this);

  // check if hBS and hUT are within the validity range
  if (hUt < 1.5 || hUt >= 10.0)
    {
      NS_LOG_WARN ("The height of the UT should be between 1.5 and 22.5 m (see TR 38.901, Table 7.4.1-1). We further assume hUT < hBS, then hUT is upper bounded by hBS, which should be 10 m");
    }

  if (hBs != 10.0)
    {
      NS_LOG_WARN ("The height of the BS should be equal to 10 m (see TR 38.901, Table 7.4.1-1)");
    }

  // NOTE The model is intended to be used for BS-UT links, however we may need to
  // compute the pathloss between two BSs or UTs, e.g., to evaluate the
  // interference. In order to apply the model, we need to retrieve the values of
  // hBS and hUT, but in these cases one of the two falls outside the validity
  // range and the warning message is printed (hBS for the UT-UT case and hUT
  // for the BS-BS case).

  // check if the distance is outside the validity range
  if (distance2D < 10.0 || distance2D > 5.0e3)
    {
      NS_LOG_WARN ("The 2D distance is outside the validity range, the pathloss value may not be accurate");
    }

  // compute the pathloss
  Ptr<MobilityModel> a;
  Ptr<MobilityModel> b;
  double plNlos = 22.4 + 35.3 * log10 (distance3D) + 21.3 * log10 (m_frequency / 1e9) - 0.3 * (hUt - 1.5);
  double loss = std::max (GetLossLos (distance2D, distance3D, hUt, hBs, a, b), plNlos);
  NS_LOG_DEBUG ("Loss " << loss);

  return loss;
}

std::pair<double, double>
ThreeGppUmiStreetCanyonPropagationLossModel::GetUtAndBsHeights (double za, double zb) const
{
  NS_LOG_FUNCTION (this);
  // TR 38.901 specifies hBS = 10 m and 1.5 <= hUT <= 22.5
  double hBs, hUt;
  if (za == 10.0)
    {
      // node A is the BS and node B is the UT
      hBs = za;
      hUt = zb;
    }
  else if (zb == 10.0)
    {
      // node B is the BS and node A is the UT
      hBs = zb;
      hUt = za;
    }
  else
    {
      // We cannot know who is the BS and who is the UT, we assume that the
      // tallest node is the BS and the smallest is the UT
      hBs = std::max (za, zb);
      hUt = std::min (za, za);
    }

  return std::pair<double, double> (hUt, hBs);
}

double
ThreeGppUmiStreetCanyonPropagationLossModel::GetShadowingStd (Ptr<MobilityModel> a, Ptr<MobilityModel> b, ChannelCondition::LosConditionValue cond) const
{
  NS_LOG_FUNCTION (this);
  NS_UNUSED (a);
  NS_UNUSED (b);
  double shadowingStd;

  if (cond == ChannelCondition::LosConditionValue::LOS)
    {
      shadowingStd = 4.0;
    }
  else if (cond == ChannelCondition::LosConditionValue::NLOS)
    {
      shadowingStd = 7.82;
    }
  else
    {
      NS_FATAL_ERROR ("Unknown channel condition");
    }

  return shadowingStd;
}

double
ThreeGppUmiStreetCanyonPropagationLossModel::GetShadowingCorrelationDistance (ChannelCondition::LosConditionValue cond) const
{
  NS_LOG_FUNCTION (this);
  double correlationDistance;

  // See 3GPP TR 38.901, Table 7.5-6
  if (cond == ChannelCondition::LosConditionValue::LOS)
    {
      correlationDistance = 10;
    }
  else if (cond == ChannelCondition::LosConditionValue::NLOS)
    {
      correlationDistance = 13;
    }
  else
    {
      NS_FATAL_ERROR ("Unknown channel condition");
    }

  return correlationDistance;
}

// ------------------------------------------------------------------------- // Hybrid Buildings

NS_OBJECT_ENSURE_REGISTERED (ThreeGppIndoorOfficePropagationLossModel);

TypeId
ThreeGppIndoorOfficePropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ThreeGppIndoorOfficePropagationLossModel")
    .SetParent<ThreeGppPropagationLossModel> ()
    .SetGroupName ("Propagation")
    .AddConstructor<ThreeGppIndoorOfficePropagationLossModel> ()
    .AddAttribute ("Frequency",
                   "The propagation frequency in Hz",
                   DoubleValue (28e9),
                   MakeDoubleAccessor (&ThreeGppIndoorOfficePropagationLossModel::m_frequency),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Environment",
                   "Environment Scenario",
                   EnumValue (UrbanEnvironment),
                   MakeEnumAccessor (&ThreeGppIndoorOfficePropagationLossModel::m_environment),
                   MakeEnumChecker (UrbanEnvironment, "Urban",
                                    SubUrbanEnvironment, "SubUrban",
                                    OpenAreasEnvironment, "OpenAreas"))
    .AddAttribute ("CitySize",
                   "Dimension of the city",
                   EnumValue (LargeCity),
                   MakeEnumAccessor (&ThreeGppIndoorOfficePropagationLossModel::m_citySize),
                   MakeEnumChecker (SmallCity, "Small",
                                    MediumCity, "Medium",
                                    LargeCity, "Large"));
  ;
  return tid;
}
ThreeGppIndoorOfficePropagationLossModel::ThreeGppIndoorOfficePropagationLossModel ()
  : ThreeGppPropagationLossModel ()
{
  NS_LOG_FUNCTION (this);

  // set a default channel condition model
  m_channelConditionModel = CreateObject<ThreeGppIndoorOpenOfficeChannelConditionModel> ();
}

ThreeGppIndoorOfficePropagationLossModel::~ThreeGppIndoorOfficePropagationLossModel ()
{
  NS_LOG_FUNCTION (this);
}

double
ThreeGppIndoorOfficePropagationLossModel::GetBpDistance (double hUt, double hBs, double distance2D) const
{
  NS_LOG_FUNCTION (this);
  NS_UNUSED (distance2D);

  // compute hE (see 3GPP TR 38.901, Table 7.4.1-1, Note 1)
  double hE = 1.0;

  // compute dBP' (see 3GPP TR 38.901, Table 7.4.1-1, Note 1)
  double distanceBp = 4 * (hBs - hE) * (hUt - hE) * m_frequency / M_C;

  return distanceBp;
}

double
ThreeGppIndoorOfficePropagationLossModel::GetLossLos (double distance2D, double distance3D, double hUt, double hBs, Ptr<MobilityModel> a,
                                             Ptr<MobilityModel> b) const
{
  NS_LOG_FUNCTION (this);
  NS_UNUSED (distance2D);
  NS_UNUSED (distance3D);
  NS_UNUSED (hUt);
  NS_UNUSED (hBs);

  Ptr<MobilityBuildingInfo> a1 = a->GetObject<MobilityBuildingInfo> ();
  Ptr<MobilityBuildingInfo> b1 = b->GetObject<MobilityBuildingInfo> ();

  double loss = 20 * std::log10 (m_frequency / 1e6 /*MHz*/) + 28 * std::log10 (a->GetDistanceFrom (b)) - 28.0; // loss = ituR123->GetLoss (a, b) ;
  // Works only for the building instantiated in 5g-periodic.cc
  double dx = std::abs (a1->GetRoomNumberX () - b1->GetRoomNumberX ());
  double dy = std::abs (a1->GetRoomNumberY () - b1->GetRoomNumberY ()); 
  double internalLoss = 5.0;  

  loss = loss + internalLoss * (dx+dy);
              
  NS_LOG_INFO (this << " I-I (same building) ITUR1238 : " << loss);
  
  loss = std::max (loss, 0.0);

  NS_LOG_DEBUG ("Loss " << loss);

  return loss;
}

double
ThreeGppIndoorOfficePropagationLossModel::GetLossNlos (double distance2D, double distance3D, double hUt, double hBs) const
{
  NS_LOG_FUNCTION (this);

  // check if hBS and hUT are within the validity range
  if (hUt < 1.5 || hUt >= 10.0)
    {
      NS_LOG_WARN ("The height of the UT should be between 1.5 and 22.5 m (see TR 38.901, Table 7.4.1-1). We further assume hUT < hBS, then hUT is upper bounded by hBS, which should be 10 m");
    }

  if (hBs != 10.0)
    {
      NS_LOG_WARN ("The height of the BS should be equal to 10 m (see TR 38.901, Table 7.4.1-1)");
    }

  // NOTE The model is intended to be used for BS-UT links, however we may need to
  // compute the pathloss between two BSs or UTs, e.g., to evaluate the
  // interference. In order to apply the model, we need to retrieve the values of
  // hBS and hUT, but in these cases one of the two falls outside the validity
  // range and the warning message is printed (hBS for the UT-UT case and hUT
  // for the BS-BS case).

  // check if the distance is outside the validity range
  if (distance2D < 10.0 || distance2D > 5.0e3)
    {
      NS_LOG_WARN ("The 2D distance is outside the validity range, the pathloss value may not be accurate");
    }

  // compute the pathloss
  Ptr<MobilityModel> a;
  Ptr<MobilityModel> b;
  double plNlos = 22.4 + 35.3 * log10 (distance3D) + 21.3 * log10 (m_frequency / 1e9) - 0.3 * (hUt - 1.5);
  double loss = std::max (GetLossLos (distance2D, distance3D, hUt, hBs, a, b), plNlos);
  NS_LOG_DEBUG ("Loss " << loss);

  return loss;
}

std::pair<double, double>
ThreeGppIndoorOfficePropagationLossModel::GetUtAndBsHeights (double za, double zb) const
{
  NS_LOG_FUNCTION (this);
  // TR 38.901 specifies hBS = 10 m and 1.5 <= hUT <= 22.5
  double hBs, hUt;
  if (za == 10.0)
    {
      // node A is the BS and node B is the UT
      hBs = za;
      hUt = zb;
    }
  else if (zb == 10.0)
    {
      // node B is the BS and node A is the UT
      hBs = zb;
      hUt = za;
    }
  else
    {
      // We cannot know who is the BS and who is the UT, we assume that the
      // tallest node is the BS and the smallest is the UT
      hBs = std::max (za, zb);
      hUt = std::min (za, za);
    }

  return std::pair<double, double> (hUt, hBs);
}

double
ThreeGppIndoorOfficePropagationLossModel::GetShadowingStd (Ptr<MobilityModel> a, Ptr<MobilityModel> b, ChannelCondition::LosConditionValue cond) const
{
  NS_LOG_FUNCTION (this);
  NS_UNUSED (a);
  NS_UNUSED (b);
  double shadowingStd;

  if (cond == ChannelCondition::LosConditionValue::LOS)
    {
      shadowingStd = 4.0;
    }
  else if (cond == ChannelCondition::LosConditionValue::NLOS)
    {
      shadowingStd = 7.82;
    }
  else
    {
      NS_FATAL_ERROR ("Unknown channel condition");
    }

  return shadowingStd;
}

double
ThreeGppIndoorOfficePropagationLossModel::GetShadowingCorrelationDistance (ChannelCondition::LosConditionValue cond) const
{
  NS_LOG_FUNCTION (this);
  double correlationDistance;

  // See 3GPP TR 38.901, Table 7.5-6
  if (cond == ChannelCondition::LosConditionValue::LOS)
    {
      correlationDistance = 10;
    }
  else if (cond == ChannelCondition::LosConditionValue::NLOS)
    {
      correlationDistance = 13;
    }
  else
    {
      NS_FATAL_ERROR ("Unknown channel condition");
    }

  return correlationDistance;
}

} // namespace ns3