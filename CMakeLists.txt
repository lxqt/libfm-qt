cmake_minimum_required(VERSION 3.0.2)
project(libfm-qt)

set(LIBFM_QT_LIBRARY_NAME "fm-qt" CACHE STRING "fm-qt")

set(LIBFM_QT_VERSION_MAJOR 0)
set(LIBFM_QT_VERSION_MINOR 11)
set(LIBFM_QT_VERSION_PATCH 1)
set(LIBFM_QT_VERSION ${LIBFM_QT_VERSION_MAJOR}.${LIBFM_QT_VERSION_MINOR}.${LIBFM_QT_VERSION_PATCH})

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# We use the libtool versioning scheme for the internal so name, "current:revision:age"
# http://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html#Updating-version-info
# https://www.sourceware.org/autobook/autobook/autobook_91.html
# http://pusling.com/blog/?p=352
# Actually, libtool uses different ways on different operating systems. So there is no
# universal way to translate a libtool version-info to a cmake version.
# We use "(current-age).age.revision" as the cmake version.
# current: 3, revision: 0, age: 0 => version: 3.0.0
set(LIBFM_QT_LIB_VERSION "3.0.0")
set(LIBFM_QT_LIB_SOVERSION "3")

set(REQUIRED_QT_VERSION "5.2")
set(REQUIRED_LIBFM_VERSION "1.2.0")
set(REQUIRED_LIBMENUCACHE_VERSION "0.4.0")

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Qt5Widgets "${REQUIRED_QT_VERSION}" REQUIRED)
find_package(Qt5LinguistTools "${REQUIRED_QT_VERSION}" REQUIRED)
find_package(Qt5X11Extras "${REQUIRED_QT_VERSION}" REQUIRED)
find_package(XCB REQUIRED)

#Note: no run-time dependency on liblxqt, just a build dependency for lxqt_translate_ts/desktop
find_package(lxqt REQUIRED)

find_package(PkgConfig)
pkg_check_modules(SYSTEM_LIBS REQUIRED
    glib-2.0
    gio-2.0
    gio-unix-2.0
)

pkg_check_modules(LIBFM REQUIRED libfm>="${REQUIRED_LIBFM_VERSION}")
pkg_check_modules(LIBMENUCACHE REQUIRED libmenu-cache>="${REQUIRED_LIBMENUCACHE_VERSION}")

option(UPDATE_TRANSLATIONS "Update source translation translations/*.ts files" OFF)
include(GNUInstallDirs)
include(GenerateExportHeader)
include(CMakePackageConfigHelpers)
include(LXQtTranslateTs)
include(LXQtTranslateDesktop)
include(LXQtCompilerSettings NO_POLICY_SCOPE)

add_definitions(-DQT_NO_KEYWORDS)

# set visibility to hidden to hide symbols, unless they're exported manually in the code
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)


set(CMAKE_AUTOMOC TRUE)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

write_basic_package_version_file(
    "${CMAKE_BINARY_DIR}/${LIBFM_QT_LIBRARY_NAME}-config-version.cmake"
    VERSION ${LIBFM_QT_LIB_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_BINARY_DIR}/${LIBFM_QT_LIBRARY_NAME}-config-version.cmake"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${LIBFM_QT_LIBRARY_NAME}"
    COMPONENT Devel
)

add_subdirectory(src)

# add Doxygen support to generate API docs
# References:
# http://majewsky.wordpress.com/2010/08/14/tip-of-the-day-cmake-and-doxygen/
# http://www.bluequartz.net/projects/EIM_Segmentation/SoftwareDocumentation/html/usewithcmakeproject.html
option(BUILD_DOCUMENTATION "Use Doxygen to create the HTML based API documentation" OFF)
if(BUILD_DOCUMENTATION)
    find_package(Doxygen REQUIRED)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile" @ONLY)
    add_custom_target(doc ALL
        ${DOXYGEN_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
    install(DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/docs"
        DESTINATION "${CMAKE_INSTALL_DOCDIR}"
        COMPONENT Devel
    )
endif()

# building tarball with CPack -------------------------------------------------
# To create a source distribution, type:
# make package_source
include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_VENDOR "")
set(CPACK_PACKAGE_VERSION_MAJOR ${LIBFM_QT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${LIBFM_QT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${LIBFM_QT_VERSION_PATCH})
set(CPACK_GENERATOR TBZ2)
set(CPACK_SOURCE_GENERATOR TBZ2)
set(CPACK_SOURCE_IGNORE_FILES /build/;.gitignore;.*~;.git;.kdev4;temp)
include(CPack)
