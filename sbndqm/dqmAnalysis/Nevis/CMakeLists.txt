simple_plugin( NevisProductionTest module
  sbndaq-artdaq-core_Overlays_SBND
  sbndaq-artdaq-core_Overlays_SBND_NevisTPC
  sbndaq_online_hiredis
  ${LARDATAOBJ}
  lardataobj_RawData
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
)

install_headers()
install_fhicl()
install_source()
