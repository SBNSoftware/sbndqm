# boost stuff
cet_find_library( BOOST_SERIALIZATION NAMES boost_serialization PATHS ENV BOOST_LIB NO_DEFAULT_PATH )
cet_find_library( BOOST_DATE_TIME     NAMES boost_date_time     PATHS ENV BOOST_LIB NO_DEFAULT_PATH )

add_subdirectory(PMT)
add_subdirectory(TPC)
add_subdirectory(Nevis)
add_subdirectory(CRT)
add_subdirectory(Meta)
add_subdirectory(example)
