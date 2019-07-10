///////////////////////////////////////////////////////////////////////////////
/// \file CRTChannelMapAlg.h
/// \brief Algorithm class for SBNDQM auxiliary detector channel mapping
///
/// Ported from AuxDetChannelMapLArIATAlg.h (Author: brebel@fnal.gov)
///
/// \version $Id:  $
/// \author mastbaum@uchicago.edu
///////////////////////////////////////////////////////////////////////////////

#ifndef SBNDQM_CRTChannelMapAlg_h
#define SBNDQM_CRTChannelMapAlg_h

#include "larcorealg/Geometry/AuxDetChannelMapAlg.h"
#include "CRTGeoObjectSorter.h"
#include "fhiclcpp/ParameterSet.h"
#include "TVector3.h"
#include <vector>

namespace geo {

  class CRTChannelMapAlg : public AuxDetChannelMapAlg {
  public:
    CRTChannelMapAlg(fhicl::ParameterSet const& p);

    void Initialize(AuxDetGeometryData_t& geodata) override;

    void Uninitialize() override;

    uint32_t PositionToAuxDetChannel(
        double const worldLoc[3],
        std::vector<geo::AuxDetGeo*> const& auxDets,
        size_t& ad,
        size_t& sv) const override;

    const TVector3 AuxDetChannelToPosition(
        uint32_t const& channel,
        std::string const& auxDetName,
        std::vector<geo::AuxDetGeo*> const& auxDets) const override;

  private:
    geo::CRTGeoObjectSorter fSorter; ///< Class to sort geo objects
  };

}  // namespace geo

#endif  // SBNDQM_CRTChannelMapAlg_h

