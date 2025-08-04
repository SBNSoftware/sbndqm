cet_build_plugin( SPECTDCStreams art::module
  LIBRARIES
  art::Utilities
  canvas::canvas  
  artdaq_core::artdaq-core_Data  
  artdaq_core::artdaq-core_Utilities
  artdaq_utilities::artdaq-utilities_Plugins
  lardataobj::RawData
  fhiclcpp::fhiclcpp
  art::Framework_Core
  messagefacility::MF_MessageLogger
  messagefacility::headers
  ROOT::Core
  cetlib::cetlib
  cetlib_except::cetlib_except

  art::Framework_Services_Registry
  art_root_io::tfile_support
  art_root_io::TFileService_service

  sbndaq_artdaq_core::sbndaq-artdaq-core_Overlays_Common 
  sbndaq_artdaq_core::sbndaq-artdaq-core_Overlays_SBND 

  sbnobj::SBND_Timing

  sbndqm_dqmAnalysis_Utils

  sbndaq_online::storage
  sbndaq_online::redis_connection
  sbndaq_online::hiredis

)

add_subdirectory(fcl)
