# basic source code CMakeLists.txt

add_subdirectory(Decode)
add_subdirectory(dqmAnalysis)
#add_subdirectory(TransferInput)

install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/AliveMonitor DESTINATION ${flavorqual_dir}/tools/)
install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/DAQConsumer  DESTINATION ${flavorqual_dir}/tools/)

install_headers()
install_source()
install_fhicl()
