art_make(
  LIB_LIBRARIES 
     canvas
     ${MF_MESSAGELOGGER}
     ${MF_UTILITIES}
     ${FHICLCPP}
     ${CETLIB}
     cetlib_except
     ${ROOT_FFTW}
     ${ROOT_GENVECTOR}
     ${ROOT_BASIC_LIB_LIST}
     ${CLHEP}
  )

install_headers()
install_fhicl()
install_source()
