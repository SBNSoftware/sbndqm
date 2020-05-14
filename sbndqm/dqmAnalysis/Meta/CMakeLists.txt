simple_plugin( ReportMetadata module
  sbndaq_online_storage
  sbndaq_online_redis_connection
  sbndaq_online_hiredis
  ${ART_UTILITIES}
  ${FHICLCPP}
  ${ART_FRAMEWORK_CORE}
  ${ROOT_BASIC_LIB_LIST}
                        ${ART_FRAMEWORK_CORE}
                        ${ART_FRAMEWORK_PRINCIPAL}
                        ${ART_FRAMEWORK_SERVICES_REGISTRY}
                        ${ART_FRAMEWORK_SERVICES_BASIC}
			art_root_io_tfile_support
			art_root_io_TFileService_service
                        art_Persistency_Common
        	${ART_FRAMEWORK_IO_SOURCES}
  artdaq-utilities_Plugins
)
install_headers()
install_fhicl()
install_source()



