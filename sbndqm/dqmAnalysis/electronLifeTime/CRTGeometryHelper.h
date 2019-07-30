///////////////////////////////////////////////////////////////////////////////
/// \file CRTGeometryHelper.h
/// \brief Auxiliary detector geometry helper service for SBNDQM geometries.
///
/// Handles SBNDQM-specific information for the generic Geometry service
/// within LArSoft. Derived from the ExptGeoHelperInterface class.
///
/// Ported from LArIATAuxDetGeometryHelper.h (Author: brebel@fnal.gov)
///
/// \verion $Id
/// \author mastbaum@uchicago.edu
///////////////////////////////////////////////////////////////////////////////

#ifndef SBNDQM_CRTExptGeoHelperInterface_h
#define SBNDQM_CRTExptGeoHelperInterface_h

#include "larcore/Geometry/AuxDetExptGeoHelperInterface.h"
#include "larcorealg/Geometry/AuxDetChannelMapAlg.h"
#include "CRTChannelMapAlg.h"
#include <memory>

namespace sbndqm {

  class CRTGeometryHelper : public geo::AuxDetExptGeoHelperInterface {
  public:

    CRTGeometryHelper(fhicl::ParameterSet const & pset,
                      art::ActivityRegistry &);

  private:

    virtual void doConfigureAuxDetChannelMapAlg(
        fhicl::ParameterSet const& sortingParameters,
        geo::AuxDetGeometryCore* geom) override;

    virtual AuxDetChannelMapAlgPtr_t doGetAuxDetChannelMapAlg() const override;

    fhicl::ParameterSet fPset; ///< Copy of configuration parameter set
    std::shared_ptr<geo::CRTChannelMapAlg> fChannelMap; ///< Channel map

  };

}  // namespace sbndqm

DECLARE_ART_SERVICE_INTERFACE_IMPL(sbndqm::CRTGeometryHelper, geo::AuxDetExptGeoHelperInterface, LEGACY)

#endif  // SBNDQM_CRTExptGeoHelperInterface_h

