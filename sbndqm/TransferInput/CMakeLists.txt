include(ProcessSmc)
install_headers()
install_source()
install_fhicl()

process_smc(SMC_LIB_SOURCE Commandable.sm)

include_directories("${CMAKE_BINARY_DIR}")

art_make_library( LIBRARY_NAME sbndqm_Application
  SOURCE
  ${SMC_LIB_SOURCE}
  Commandable.cc
  LIBRARIES
  # artdaq_DAQrate
  # artdaq_Application_Routing
  sbndqm_DAQdata
  artdaq-core_Data
  artdaq-core_Utilities
  ${MF_MESSAGELOGGER}
  ${CETLIB}
  ${CETLIB_EXCEPT}
  canvas
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${FHICLCPP}
)

art_make_library( LIBRARY_NAME sbndqm_ExternalComms
  SOURCE 
  CommanderInterface.cc
  MakeCommanderPlugin.cc
  LIBRARIES 
  # artdaq_DAQdata
  sbndqm_DAQdata
  sbndqm_Application
  ${FHICLCPP}
  ${CETLIB}
)

art_make_library( LIBRARY_NAME sbndqm_DAQdata
  SOURCE 
    Globals.cc
    PortManager.cc
    TCPConnect.cc
  LIBRARIES
    ${ARTDAQ-CORE_DATA}
    ${ARTDAQ-CORE_GENERATORS}
    artdaq-utilities_Plugins
    ${FHICLCPP}
    ${CETLIB}
    ${CETLIB_EXCEPT}
    # ${MESSAGEFACILITY_LIBS}
    ${MF_MESSAGELOGGER}
    ${MF_UTILITIES}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
)

art_make_library( LIBRARY_NAME sbndqm_TransferPlugins
  SOURCE
  MakeTransferPlugin.cc
  TransferInterface.cc
  LIBRARIES
  artdaq-core_Utilities
  artdaq-core_Data
  # artdaq_DAQdata
  sbndqm_DAQdata
)

art_make_library( LIBRARY_NAME sbndqm_ArtModules
  SOURCE 
  TransferWrapper.cc
  LIBRARIES
  artdaq-core_Data
  artdaq-core_Data_dict
  # artdaq_DAQrate
  # artdaq_DAQdata
  # artdaq_Application
  # artdaq_ExternalComms
  sbndqm_DAQdata
  sbndqm_Application
  sbndqm_ExternalComms
  sbndqm_TransferPlugins
  art_Framework_Core
  art_Framework_IO_Sources
  art_Framework_IO_Root
  art_Framework_IO_RootVersion
  art_Framework_IO
  art_Framework_Principal
  # artdaq_ArtModules_NetMonTransportService_service
  art_Persistency_Provenance
  #  art_Persistency_RootDB
  canvas
  MF_MessageLogger
  ${ROOT_CINTEX}
  ${ROOT_RIO}
  ${ROOT_TREE}
  ${ROOT_CORE}
  ${ROOT_NET}
  ${ROOT_REFLEX}
  ${Boost_DATE_TIME_LIBRARY}
)
  

simple_plugin(TransferInput "source"
  artdaq-core_Data
  artdaq-core_Data_dict
  # artdaq_TransferPlugins
  # artdaq_ArtModules
  sbndqm_TransferPlugins
  sbndqm_ArtModules
  ${TBB}
)


