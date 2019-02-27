include_directories($ENV{JSONCPP_LIB})

find_library(JSONCPP jsoncpp HINTS $ENV{JSONCPP_LIB})

cet_make_library(LIBRARY_NAME sbndqm_redis_config
  SOURCE
    ConfigureRedis.cc 
  LIBRARIES
    ${FHICLCPP}
    ${CETLIB}
    ${CETLIB_EXCEPT}
    ${MF_MESSAGELOGGER}
    ${MF_UTILITIES}
    sbndaq_redis_plugin_hiredis
    ${JSONCPP}
)

install_source()
install_headers()
