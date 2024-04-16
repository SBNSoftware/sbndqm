art_make_library(
  LIBRARIES
    sbndaq_online::storage
    messagefacility::MF_MessageLogger
    fhiclcpp::fhiclcpp
    ROOT::Hist
    TRACE::tracelib
)

install_headers()
install_source()
