cet_build_plugin( ReportMetadata art::module
  LIBRARIES
  sbndaq_online::storage
  sbndaq_online::redis_connection
  sbndaq_online::hiredis
  art::Utilities
  fhiclcpp::fhiclcpp
  art::Framework_Core
  ROOT::Core
  art::Framework_Principal
  art::Framework_Services_Registry
  art_root_io::tfile_support
  art_root_io::TFileService_service
  art::Persistency_Common
  art::Framework_IO_Sources
  artdaq_utilities::artdaq-utilities_Plugins
)
install_headers()
install_fhicl()
install_source()



