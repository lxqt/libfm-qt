install(FILES
    "archivers.list"
    "terminals.list"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/libfm-qt6"
)

install(FILES
    "libfm-qt6-mimetypes.xml"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/mime/packages"
)
