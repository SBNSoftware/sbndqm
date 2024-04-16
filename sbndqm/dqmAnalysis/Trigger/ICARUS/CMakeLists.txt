cet_build_plugin( TriggerMonitorWithDecoder art::module
  LIBRARIES
  art::Utilities
  canvas::canvas
  art::Framework_Services_Registry
  lardataobj::RawData
  lardataobj::Simulation
  fhiclcpp::fhiclcpp
  messagefacility::MF_MessageLogger
  messagefacility::headers
  art::Framework_Core
  ROOT::Core
  art_root_io::tfile_support
  art_root_io::TFileService_service
  sbndaq_artdaq_core::sbndaq-artdaq-core_Overlays_Common 
  artdaq_core::artdaq-core_Utilities
  artdaq_utilities::artdaq-utilities_Plugins
  sbndaq_online::storage
  sbndaq_online::redis_connection
  sbndaq_online::hiredis
  sbndqm_Decode_PMT_PMTDecodeData
  sbnobj::Common_Trigger
  sbndqm::dqmAnalysis_Utilities
  larcorealg::headers
)

add_subdirectory(fcl)
