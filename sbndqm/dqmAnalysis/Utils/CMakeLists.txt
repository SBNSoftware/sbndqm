cet_make_library(
	LIBRARY_NAME
    	sbndqm_dqmAnalysis_Utils

	SOURCE 
	SBNDHLTFilterUtils.cc
	
	LIBRARIES
        messagefacility::MF_MessageLogger
        art::Utilities
        canvas::canvas
        artdaq_core::artdaq-core_Data
        artdaq_core::artdaq-core_Utilities
        sbndaq_artdaq_core::sbndaq-artdaq-core_Overlays_Common
        sbndaq_artdaq_core::sbndaq-artdaq-core_Overlays_SBND
)

install_headers()
install_source()
