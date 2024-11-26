add_subdirectory(DQMChannelMap)
# this is borrowed from sbndaq-artdaq-core to make the c7 compiler happy

add_definitions(-Wno-nested-anon-types)

cet_build_plugin( SBNDDQMTPCDecoder art::module
  SOURCE SBNDDQMTPCDecoder_module.cc
  LIBRARIES
  sbndaq_artdaq_core::sbndaq-artdaq-core_Overlays_SBND
  sbndaq_artdaq_core::sbndaq-artdaq-core_Overlays_SBND_NevisTPC
  lardataobj::RawData
  artdaq_core::artdaq-core_Utilities
  art::Utilities
  fhiclcpp::fhiclcpp
  messagefacility::MF_MessageLogger
  art::Framework_Core
  ROOT::Core
  ROOT::Tree 
  sbndqm_Decode_TPC_SBND_DQMChannelMap_TPCDQMChannelMapService_service
  #sbndcode_ChannelMaps_TPC_TPCChannelMapService_service
)

install_headers()
install_fhicl()
install_source()
art_dictionary()

