add_subdirectory(DATA)
add_subdirectory(Fragments)
#cet_make_library( LIBRARY_NAME sbndqm_Decode_CRT
#        SOURCE
#                CRT_RawDecoder_module.cc
#)
cet_build_plugin(CRTRawDecoder art::module
                       
			
  			sbndaq_online_storage
  			sbndaq_online_hiredis
  			artdaq_utilities::artdaq-utilities_Plugins
			artdaq_core::artdaq-core_Data
			artdaq_core::artdaq-core_Utilities
			icarusCRT-raw-data_Overlays
                        art::Framework_Core
                        art::Framework_Principal
                        art::Framework_Services_Registry
                        art_root_io::tfile_support
                        ROOT::Core
                        art_root_io::TFileService_service
                        ART_PERSISTENCY_PROVENANCE
                        messagefacility::MF_MessageLogger
                        CETLIB
                        ROOT_BASIC_LIB_LIST                 
                        BASENAME_ONLY
)
cet_build_plugin(CRTOnlineMonitor art::module
                       
			
  			sbndaq_online_storage
  			sbndaq_online_hiredis
  			artdaq_utilities::artdaq-utilities_Plugins
			artdaq_core::artdaq-core_Data
			artdaq_core::artdaq-core_Utilities
			icarusCRT-raw-data_Overlays
                        art::Framework_Core
                        art::Framework_Principal
                        art::Framework_Services_Registry
                        art_root_io::tfile_support
                        ROOT::Core
                        art_root_io::TFileService_service
                        ART_PERSISTENCY_PROVENANCE
                        messagefacility::MF_MessageLogger
                        CETLIB
                        ROOT_BASIC_LIB_LIST                 
                        BASENAME_ONLY
)
install_fhicl()
install_scripts()
install_headers()
install_source()


