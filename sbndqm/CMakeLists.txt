# basic source code CMakeLists.txt
find_ups_product(icarus_artdaq_base)

add_subdirectory(dqmAnalysis)
add_subdirectory(MetricManagerShim)
add_subdirectory(MetricConfig)
add_subdirectory(TransferInput)

install_headers()
install_source()
install_fhicl()
