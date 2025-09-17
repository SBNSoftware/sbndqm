cet_build_plugin( FragmentDQMAna art::module
  LIBRARIES
  art::Framework_Services_Registry
  ROOT::Core
  sbndaq_artdaq_core::sbndaq-artdaq-core_Overlays_Common
  artdaq_core::artdaq-core_Data
  artdaq_utilities::artdaq-utilities_Plugins
  sbndaq_online::storage
  sbndaq_online::hiredis
  sbndqm_Decode_Mode
)

