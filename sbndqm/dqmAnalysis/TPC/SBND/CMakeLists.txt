include_directories( ${FFTW_INCLUDE_DIR} )

cet_build_plugin( OfflineAnalysisSBND art::module
  LIBRARIES
  tpcAnalysis_SBN
  lardataobj::RawData
  lardataobj::RecoBase
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

cet_build_plugin( TPCWaveformCreatorSBND art::module
  LIBRARIES
  sbndaq_online::hiredis
  tpcAnalysis_SBN
  lardataobj::RawData
  lardataobj::RecoBase
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

cet_build_plugin( OnlineAnalysisSBND art::module
  LIBRARIES
  sbndaq_online::storage
  sbndaq_online::redis_connection
  sbndaq_online::hiredis
  tpcAnalysis_SBN
  lardataobj::RawData
  lardataobj::RecoBase
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

cet_build_plugin( TPCWaveformAndFftRedisSBND art::module
  LIBRARIES
  tpcAnalysis_SBN
  lardataobj::RawData
  lardataobj::RecoBase
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

cet_build_plugin( OnlineEvdSBND art::module
        ROOT::Core
        lardataobj::RecoBase
        larcore::Geometry_Geometry_service
        artdaq_utilities::artdaq-utilities_Plugins
        sbndaq_online_storage
        sbndaq_online_redis_connection
        sbndaq_online_hiredis
        sbndqm_Decode_TPC_SBND_DQMChannelMap_TPCDQMChannelMapService_service
        #sbndcode_ChannelMaps_TPC_TPCChannelMapService_service
)

cet_build_plugin( OfflineEvdSBND art::module
        ROOT::Core
        lardataobj::RecoBase
        larcore::Geometry_Geometry_service
        artdaq_utilities::artdaq-utilities_Plugins
        sbndaq_online_storage
        sbndaq_online_redis_connection
        sbndaq_online_hiredis
        sbndqm_Decode_TPC_SBND_DQMChannelMap_TPCDQMChannelMapService_service
        #sbndcode_ChannelMaps_TPC_TPCChannelMapService_service
)

install_headers()
install_fhicl()
install_source()



