
cet_build_plugin( CAENV1740StreamsSBND art::module
  LIBRARIES
  lardataobj::RawData
  larana::OpticalDetector_OpHitFinder
  sbndaq_online::storage
  sbndaq_online::redis_connection
  sbndaq_online::hiredis
  tpcAnalysis_SBN

)

add_subdirectory(fcl)
install_fhicl()
