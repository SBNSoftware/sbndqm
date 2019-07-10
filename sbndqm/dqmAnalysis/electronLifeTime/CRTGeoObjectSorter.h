///////////////////////////////////////////////////////////////////////////////
/// \file CRTGeoObjectSorter.h
/// \brief Interface to algorithm class for sorting of AuxDetGeo objects
///
/// Ported from AuxDetGeoObjectSorterLArIAT.h (Author: brebel@fnal.gov)
///
/// \version $Id:  $
/// \author mastbaum@uchicago.edu
///////////////////////////////////////////////////////////////////////////////

#ifndef SBNDQM_CRTGeoObjectSorter_h
#define SBNDQM_CRTGeoObjectSorter_h

#include <vector>

#include "larcorealg/Geometry/AuxDetGeoObjectSorter.h"

namespace geo {

  class CRTGeoObjectSorter : public AuxDetGeoObjectSorter {
  public:

    CRTGeoObjectSorter(fhicl::ParameterSet const& p);

    ~CRTGeoObjectSorter();

    void SortAuxDets (std::vector<geo::AuxDetGeo*>& adgeo) const;
    void SortAuxDetSensitive(std::vector<geo::AuxDetSensitiveGeo*>& adsgeo) const;

  };

}

#endif  // SBNDQM_CRTGeoObjectSorter_h

