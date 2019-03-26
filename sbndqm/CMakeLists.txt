# basic source code CMakeLists.txt

add_subdirectory(dqmAnalysis)
add_subdirectory(MetricManagerShim)
add_subdirectory(MetricConfig)
add_subdirectory(TransferInput)

install_headers()
install_source()
install_fhicl()
