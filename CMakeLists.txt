include(FetchContent)
cmake_minimum_required(VERSION 3.22)

project(transit-peek)

set(PLASMA_TARGET_VERSION "6.6")
set(QT_MIN_VERSION "6.10")
set(KF_MIN_VERSION "6.22")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
set(CMAKE_CXX_STANDARD 20)

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)
include(ExternalProject)
include(FetchContent)

set(EXPORT_COMPILE_COMMANDS ON)

set(TRANSIT_PEEK_PROTOBUF_VERSION "v34.1")
set(TRANSIT_PEEK_PROTOBUF_PREFIX "${CMAKE_BINARY_DIR}/_deps/protobuf")
set(TRANSIT_PEEK_PROTOBUF_INSTALL_DIR "${TRANSIT_PEEK_PROTOBUF_PREFIX}/install" CACHE PATH "Bundled protobuf install directory")
set(TRANSIT_PEEK_PROTOBUF_CONFIG "${TRANSIT_PEEK_PROTOBUF_INSTALL_DIR}/lib/cmake/protobuf/protobuf-config.cmake" CACHE FILEPATH "Bundled protobuf config file")

ExternalProject_Add(transit_peek_protobuf
  PREFIX "${TRANSIT_PEEK_PROTOBUF_PREFIX}"
  URL "https://github.com/protocolbuffers/protobuf/archive/refs/tags/${TRANSIT_PEEK_PROTOBUF_VERSION}.tar.gz"
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  INSTALL_DIR "${TRANSIT_PEEK_PROTOBUF_INSTALL_DIR}"
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DCMAKE_INSTALL_LIBDIR=lib
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    -DCMAKE_BUILD_TYPE=MinSizeRel
    -Dprotobuf_BUILD_SHARED_LIBS=OFF
    -Dprotobuf_BUILD_TESTS=OFF
    -Dprotobuf_BUILD_EXAMPLES=OFF
    -Dprotobuf_BUILD_PROTOC_BINARIES=ON
    -Dprotobuf_FORCE_FETCH_DEPENDENCIES=ON
  BUILD_BYPRODUCTS
    "${TRANSIT_PEEK_PROTOBUF_INSTALL_DIR}/bin/protoc"
    "${TRANSIT_PEEK_PROTOBUF_INSTALL_DIR}/lib/libprotobuf.a"
)

find_package(Qt6 ${QTMIN_VERSION} CONFIG REQUIRED COMPONENTS
    Quick
    Core
    Qml
    Sql
)
find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    I18n
    Config
    KIO
    Archive
    KCMUtils
)
find_package(Plasma ${PLASMA_TARGET_VERSION} REQUIRED)

FetchContent_Declare(
  csv  
  GIT_REPOSITORY https://github.com/vincentlaucsb/csv-parser.git
  GIT_SHALLOW TRUE 
  GIT_TAG 5.2.0
)
FetchContent_MakeAvailable(csv)

add_subdirectory(src)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)