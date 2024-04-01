include_directories( ${FFTW_INCLUDE_DIR} )

# filter modules for running online monitoring
add_subdirectory(OnlineFilters)

# install fcl
add_subdirectory(fcl)

art_dictionary(DICTIONARY_LIBRARIES tpcAnalysis_SBN)

cet_make_library( LIBRARY_NAME tpcAnalysis_SBN
	SOURCE  Analysis.cc
		FFT.cc
		Noise.cc
		PeakFinder.cc
		ChannelData.cc
	LIBRARIES
		sbndaq_artdaq_core::sbndaq-artdaq-core_Overlays_SBND_NevisTPC
	        sbndqm::Decode_Mode
		lardata::Utilities
		lardataobj::RawData
		lardataobj::RecoBase
		art::Framework_Principal
		art::Framework_Services_Registry
		art_root_io::tfile_support
		art_root_io::TFileService_service
		art_root_io::art_root_io
                art::Persistency_Common
		art::Persistency_Provenance
        	art::Framework_IO_Sources
       		canvas::canvas
		art::Framework_Services_FileServiceInterfaces
		art::Utilities
		fhiclcpp::fhiclcpp
		ROOT::Core
		FFTW3::FFTW3
                larcore::Geometry_Geometry_service
                larcorealg::Geometry
                lardataobj::Simulation
                lardata::DetectorInfoServices_DetectorClocksServiceStandard_service
		art::Framework_Core
		art::Framework_Services_Optional_RandomNumberGenerator_service
		messagefacility::MF_MessageLogger
		messagefacility::headers
                cetlib::cetlib
		cetlib_except::cetlib_except
		CLHEP::Random
		ROOT::Geom
		ROOT::XMLIO
		ROOT::Gdml
		Boost::system
)

cet_build_plugin( OfflineAnalysis art::module
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
)

cet_build_plugin( TPCWaveformCreator art::module
  LIBRARIES
  sbndaq_online::hiredis
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
)

cet_build_plugin( OnlineAnalysis art::module
  LIBRARIES
  sbndaq_online::storage
  sbndaq_online::redis_connection
  sbndaq_online::hiredis
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



