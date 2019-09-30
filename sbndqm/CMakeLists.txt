# basic source code CMakeLists.txt

add_subdirectory(dqmAnalysis)
add_subdirectory(MetricManagerShim)
add_subdirectory(MetricConfig)
add_subdirectory(TransferInput)
add_subdirectory(DatabaseStorage)

install_headers()
install_source()
install_fhicl()
