include_directories( ${FFTW_INCLUDE_DIR} )

# install fcl
add_subdirectory(fcl)

cet_build_plugin( OfflineAnalysis art::module
  LIBRARIES
  tpcAnalysis_SBN
  ${LARDATAOBJ}
  lardataobj::RawData
  art::Utilities
  fhiclcpp::fhiclcpp
  art::Framework_Core
  ROOT::Core
  art::Framework_Principal
  art::Framework_Services_Registry
  art_root_io::tfile_support
  art_root_io::TFileService_service
  art_root_io::art_root_io
  art::Persistency_Common
  art::Framework_IO_Sources
)

cet_build_plugin( TPCWaveformCreator art::module
  LIBRARIES
  sbndaq_online::hiredis
  tpcAnalysis_SBN
  ${LARDATAOBJ}
  lardataobj::RawData
  art::Utilities
  fhiclcpp::fhiclcpp
  art::Framework_Core
  ROOT::Core
  art::Framework_Principal
  art::Framework_Services_Registry
  art_root_io::tfile_support
  art_root_io::TFileService_service
  art_root_io::art_root_io
  art::Persistency_Common
  art::Framework_IO_Sources
)

cet_build_plugin( OnlineAnalysis art::module
  LIBRARIES
  sbndaq_online::storage
  sbndaq_online::redis_connection
  sbndaq_online::hiredis
  tpcAnalysis_SBN
  ${LARDATAOBJ}
  lardataobj::RawData
  art::Utilities
  fhiclcpp::fhiclcpp
  art::Framework_Core
  ROOT::Core
  art::Framework_Principal
  art::Framework_Services_Registry
  art_root_io::tfile_support
  art_root_io::TFileService_service
  art_root_io::art_root_io
  art::Persistency_Common
  art::Framework_IO_Sources
  artdaq_utilities::artdaq-utilities_Plugins
)

cet_build_plugin( TPCPurityDQMSender art::module
  LIBRARIES
  sbndaq_online::storage
  sbndaq_online::redis_connection
  sbndaq_online::hiredis
  tpcAnalysis_SBN
  sbnobj::Common_Analysis
  lardataobj::RawData
  art::Utilities
  fhiclcpp::fhiclcpp
  art::Framework_Core
  ROOT::Core
  art::Framework_Principal
  art::Framework_Services_Registry
  art_root_io::tfile_support
  art_root_io::TFileService_service
  art_root_io::art_root_io
  art::Persistency_Common
  art::Framework_IO_Sources
  artdaq_utilities::artdaq-utilities_Plugins
)
cet_build_plugin( TPCWaveformAndFftRedis art::module
  LIBRARIES
  tpcAnalysis_SBN
  lardataobj::RawData
  art::Utilities
  fhiclcpp::fhiclcpp
  art::Framework_Core
  ROOT::Core
  art::Framework_Principal
  art::Framework_Services_Registry
  art_root_io::tfile_support
  art_root_io::TFileService_service
  art_root_io::art_root_io
  art::Persistency_Common
  art::Framework_IO_Sources
  artdaq_utilities::artdaq-utilities_Plugins
  sbndaq_online::storage
  sbndaq_online::hiredis
  sbndaq_online::redis_connection
)

install_headers()
install_fhicl()
install_source()



