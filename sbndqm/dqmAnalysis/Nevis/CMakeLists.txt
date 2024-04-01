cet_build_plugin( NevisProductionTest art::module
  LIBRARIES
  sbndaq_artdaq_core::sbndaq-artdaq-core_Overlays_SBND
  sbndaq_artdaq_core::sbndaq-artdaq-core_Overlays_SBND_NevisTPC
  sbndaq_online_hiredis
  lardataobj::RawData
  art::Utilities
  canvas::canvas  
  art::Framework_Core
  art::Framework_Principal
  art::Framework_Services_Registry
  messagefacility::MF_MessageLogger
  messagefacility::headers
  ROOT::Core
  cetlib::cetlib
  cetlib_except::cetlib_except
  art_root_io::tfile_support
  art_root_io::TFileService_service
  art::Persistency_Common
  art::Framework_IO_Sources
)

install_headers()
install_fhicl()
install_source()
