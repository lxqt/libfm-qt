include_directories(
    "${LIBFM_INCLUDE_DIRS}"
    "${LIBFM_INCLUDEDIR}/libfm" # to workaround incorrect #include in fm-actions.
    "${LIBMENUCACHE_INCLUDE_DIRS}"
    "${SYSTEM_LIBS_INCLUDE_DIRS}"
)

link_directories(
    "${LIBFM_LIBRARY_DIRS}"
    "${LIBMENUCACHE_LIBRARY_DIRS}"
    "${SYSTEM_LIBS_LIBRARY_DIRS}"
)

set(libfm_SRCS
    libfmqt.cpp
    bookmarkaction.cpp
    sidepane.cpp
    icontheme.cpp
    filelauncher.cpp
    foldermodel.cpp
    foldermodelitem.cpp
    cachedfoldermodel.cpp
    proxyfoldermodel.cpp
    folderview.cpp
    folderitemdelegate.cpp
    createnewmenu.cpp
    filemenu.cpp
    foldermenu.cpp
    filepropsdialog.cpp
    applaunchcontext.cpp
    placesview.cpp
    placesmodel.cpp
    placesmodelitem.cpp
    dirtreeview.cpp
    dirtreemodel.cpp
    dirtreemodelitem.cpp
    dnddest.cpp
    mountoperation.cpp
    mountoperationpassworddialog.cpp
    mountoperationquestiondialog.cpp
    fileoperation.cpp
    fileoperationdialog.cpp
    renamedialog.cpp
    pathedit.cpp
    colorbutton.cpp
    fontbutton.cpp
    browsehistory.cpp
    utilities.cpp
    dndactionmenu.cpp
    editbookmarksdialog.cpp
    thumbnailloader.cpp
    path.cpp
    execfiledialog.cpp
    appchoosercombobox.cpp
    appmenuview.cpp
    appchooserdialog.cpp
    filesearchdialog.cpp
    fm-search.c # might be moved to libfm later
    xdndworkaround.cpp
)

set(libfm_UIS
    file-props.ui
    file-operation-dialog.ui
    rename-dialog.ui
    mount-operation-password.ui
    edit-bookmarks.ui
    exec-file.ui
    app-chooser-dialog.ui
    filesearch.ui
)

qt5_wrap_ui(libfm_UIS_H ${libfm_UIS})

# add translation for libfm-qt
lxqt_translate_ts(QM_FILES
    UPDATE_TRANSLATIONS ${UPDATE_TRANSLATIONS}
    SOURCES ${libfm_SRCS} ${libfm_UIS}
    INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/libfm-qt/translations"
    PULL_TRANSLATIONS ${PULL_TRANSLATIONS}
    CLEAN_TRANSLATIONS ${CLEAN_TRANSLATIONS}
    TRANSLATIONS_REPO ${TRANSLATIONS_REPO}
    TRANSLATIONS_REFSPEC ${TRANSLATIONS_REFSPEC}
)

add_library(${LIBFM_QT_LIBRARY_NAME} SHARED
    ${libfm_SRCS}
    ${libfm_UIS_H}
    ${QM_FILES}
)

set_property(
    TARGET ${LIBFM_QT_LIBRARY_NAME} APPEND
    PROPERTY COMPILE_DEFINITIONS
    LIBFM_DATA_DIR="${CMAKE_INSTALL_FULL_DATADIR}/libfm-qt"
)

# only turn on custom actions support if it is enabled in libfm.
if(EXISTS "${LIBFM_INCLUDEDIR}/libfm/fm-actions.h")
    set_property(TARGET ${LIBFM_QT_LIBRARY_NAME} APPEND PROPERTY COMPILE_DEFINITIONS CUSTOM_ACTIONS)
endif()

install(EXPORT
    "${LIBFM_QT_LIBRARY_NAME}-targets"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${LIBFM_QT_LIBRARY_NAME}"
    COMPONENT Devel
)

target_link_libraries(${LIBFM_QT_LIBRARY_NAME}
    Qt5::Widgets
    Qt5::X11Extras
    ${LIBFM_LIBRARIES}
    ${LIBMENUCACHE_LIBRARIES}
    ${SYSTEM_LIBS_LIBRARIES}
)

# set libtool soname
set_target_properties(${LIBFM_QT_LIBRARY_NAME} PROPERTIES
    VERSION ${LIBFM_QT_LIB_VERSION}
    SOVERSION ${LIBFM_QT_LIB_SOVERSION}
)

target_include_directories(${LIBFM_QT_LIBRARY_NAME}
    PRIVATE "${LIB_XCB_INDLUDE_DIRS}"
    PRIVATE "${Qt5Gui_PRIVATE_INCLUDE_DIRS}"
    INTERFACE "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${LIBFM_QT_LIBRARY_NAME}_export.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libfm-qt"
    COMPONENT Devel
)

file(GLOB libfm_HS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
# install include header files (FIXME: can we make this cleaner? should dir name be versioned?)
install(FILES ${libfm_HS}
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libfm-qt"
    COMPONENT Devel
)

generate_export_header(${LIBFM_QT_LIBRARY_NAME}
    EXPORT_MACRO_NAME LIBFM_QT_API
)

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/fm-qt-config.cmake.in"
    "${PROJECT_BINARY_DIR}/install/${LIBFM_QT_LIBRARY_NAME}-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${LIBFM_QT_LIBRARY_NAME}"
)

install(FILES
    "${PROJECT_BINARY_DIR}/install/${LIBFM_QT_LIBRARY_NAME}-config.cmake"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${LIBFM_QT_LIBRARY_NAME}"
    COMPONENT Devel
)

# FIXME: add libtool version to the lib (soname) later.
# FIXME: only export public symbols

install(TARGETS ${LIBFM_QT_LIBRARY_NAME}
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    EXPORT "${LIBFM_QT_LIBRARY_NAME}-targets"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    PUBLIC_HEADER
    COMPONENT Runtime
)

export(TARGETS ${LIBFM_QT_LIBRARY_NAME}
    FILE "${CMAKE_BINARY_DIR}/${LIBFM_QT_LIBRARY_NAME}-targets.cmake"
    EXPORT_LINK_INTERFACE_LIBRARIES
)

# install a pkgconfig file for libfm-qt
set(REQUIRED_QT "Qt5Widgets >= ${REQUIRED_QT_VERSION} Qt5X11Extras >= ${REQUIRED_QT_VERSION}")
configure_file(libfm-qt.pc.in lib${LIBFM_QT_LIBRARY_NAME}.pc @ONLY)
# FreeBSD loves to install files to different locations
# http://www.freebsd.org/doc/handbook/dirstructure.html
if(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/lib${LIBFM_QT_LIBRARY_NAME}.pc"
        DESTINATION libdata/pkgconfig
        COMPONENT Devel
    )
else()
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/lib${LIBFM_QT_LIBRARY_NAME}.pc"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
        COMPONENT Devel
    )
endif()

# prevent the generated files from being deleted during make cleaner
set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM true)
