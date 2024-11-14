cet_build_plugin( SnapshotFilter art::module 
  LIBRARIES
  lardataobj::RawData
  art::Utilities
  canvas::canvas  
  art::Framework_Core
  messagefacility::MF_MessageLogger
  messagefacility::headers
  ROOT::Core
  cetlib::cetlib
  cetlib_except::cetlib_except
)

install_headers()
install_source()

