cet_make_library(LIBRARY_NAME sbndqm_datbase_storage
  SOURCE
    Histogram.cc Binary.cc
  LIBRARIES
    sbndaq_redis_plugin_hiredis
    ${ROOT_BASIC_LIB_LIST}
)

install_source()
install_headers()
