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

simple_plugin(Shmem "transfer"
  #artdaq_TransferPlugins
  sbndqm_TransferPlugins
  artdaq-core_Core
)

find_library( XMLRPC_SERVER_ABYSSPP NAMES xmlrpc_server_abyss++ PATHS $ENV{XMLRPC_C_LIBRARY} NO_DEFAULT_PATH )
find_library( XMLRPC_SERVER_ABYSS NAMES xmlrpc_server_abyss PATHS $ENV{XMLRPC_C_LIBRARY} NO_DEFAULT_PATH )
find_library( XMLRPC_SERVERPP NAMES xmlrpc_server++ PATHS $ENV{XMLRPC_C_LIBRARY} NO_DEFAULT_PATH )
find_library( XMLRPC_SERVER NAMES xmlrpc_server PATHS $ENV{XMLRPC_C_LIBRARY} NO_DEFAULT_PATH )
find_library( XMLRPC_ABYSS NAMES xmlrpc_abyss PATHS $ENV{XMLRPC_C_LIBRARY} NO_DEFAULT_PATH )
find_library( XMLRPC_CLIENTPP NAMES xmlrpc_client++ PATHS $ENV{XMLRPC_C_LIBRARY} NO_DEFAULT_PATH )
find_library( XMLRPCPP NAMES xmlrpc++ PATHS $ENV{XMLRPC_C_LIBRARY} NO_DEFAULT_PATH )
find_library( XMLRPC NAMES xmlrpc PATHS $ENV{XMLRPC_C_LIBRARY} NO_DEFAULT_PATH )
find_library( XMLRPC_UTIL NAMES xmlrpc_util PATHS $ENV{XMLRPC_C_LIBRARY} NO_DEFAULT_PATH )
find_library( XMLRPC_UTILPP NAMES xmlrpc_util++ PATHS $ENV{XMLRPC_C_LIBRARY} NO_DEFAULT_PATH )
# XMLRPC_UTILPP is new to v1_39_07:
if(NOT XMLRPC_UTILPP)
        set(XMLRPC_UTILPP "")
endif()
find_library( XMLRPC_XMLPARSE NAMES xmlrpc_xmlparse PATHS $ENV{XMLRPC_C_LIBRARY} NO_DEFAULT_PATH )
find_library( XMLRPC_XMLTOK NAMES xmlrpc_xmltok PATHS $ENV{XMLRPC_C_LIBRARY} NO_DEFAULT_PATH )

simple_plugin(xmlrpc "commander"
  # artdaq_ExternalComms
  sbndqm_ExternalComms
  ${FHICLCPP}
  ${XMLRPC_SERVER_ABYSSPP}
  ${XMLRPC_SERVER_ABYSS}
  ${XMLRPC_SERVERPP}
  ${XMLRPC_CLIENT}
  ${XMLRPC_CLIENTPP}
  ${XMLRPC_SERVER}
  ${XMLRPC_ABYSS}
  ${XMLRPCPP}
  ${XMLRPC}
  ${XMLRPC_UTIL}
  ${XMLRPC_UTILPP}
  ${XMLRPC_XMLPARSE}
  ${XMLRPC_XMLTOK}
)

