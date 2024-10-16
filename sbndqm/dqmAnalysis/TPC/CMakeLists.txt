include_directories( ${FFTW_INCLUDE_DIR} )

# install fcl
add_subdirectory(OnlineFilters)
add_subdirectory(ICARUS)
add_subdirectory(SBND)

art_dictionary(DICTIONARY_LIBRARIES tpcAnalysis_SBN)

cet_make_library( LIBRARY_NAME tpcAnalysis_SBN
	SOURCE  Analysis.cc
                AnalysisSBND.cc
		FFT.cc
		Noise.cc
		PeakFinder.cc
		ChannelData.cc
		ChannelDataSBND.cc
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

install_headers()
install_fhicl()
install_source()



