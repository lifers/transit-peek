add_compile_options(-fexceptions)

add_definitions(-DTRANSLATION_DOMAIN="plasma_applet_id.lifers.transit_peek")

plasma_add_applet(id.lifers.transit_peek
    QML_SOURCES
        qml/config.qml
        qml/main.qml
        qml/ArrivalsList.qml
        qml/configStops.qml
        qml/PresetMaker.qml
    CPP_SOURCES
        TransitFeed.hpp
        ArrivalsModel.hpp
        PresetModel.hpp
        DictionaryStore.hpp
        SearchModel.hpp
    RESOURCES
        main.xml
    GENERATE_APPLET_CLASS
)
set(protobuf_MODULE_COMPATIBLE ON)
if(NOT EXISTS "${TRANSIT_PEEK_PROTOBUF_CONFIG}")
    message(FATAL_ERROR
        "Bundled protobuf is not installed yet. Build target 'transit_peek_protobuf' first, then rerun CMake configure."
    )
endif()

set(Protobuf_DIR "${TRANSIT_PEEK_PROTOBUF_INSTALL_DIR}/lib/cmake/protobuf" CACHE PATH "Bundled protobuf package directory" FORCE)
set(absl_DIR "${TRANSIT_PEEK_PROTOBUF_INSTALL_DIR}/lib/cmake/absl" CACHE PATH "Bundled absl package directory" FORCE)
find_package(Protobuf REQUIRED CONFIG
    PATHS "${TRANSIT_PEEK_PROTOBUF_INSTALL_DIR}"
    NO_DEFAULT_PATH
)
message(STATUS "Using bundled static protobuf from ${TRANSIT_PEEK_PROTOBUF_INSTALL_DIR}")
# Prefer bundled package configs (protobuf + its fetched deps like absl)
set(CMAKE_PREFIX_PATH "${TRANSIT_PEEK_PROTOBUF_INSTALL_DIR}/lib/cmake;${CMAKE_PREFIX_PATH}" CACHE STRING "Prefer bundled protobuf/absl package directory" FORCE)

target_compile_options(csv PRIVATE -fexceptions)
if(TARGET csv_no_simd)
    target_compile_options(csv_no_simd PRIVATE -fexceptions)
endif()

target_compile_features(id.lifers.transit_peek PUBLIC cxx_std_20)
add_dependencies(id.lifers.transit_peek transit_peek_protobuf)
target_link_libraries(id.lifers.transit_peek PRIVATE
    Qt6::Gui
    Qt6::Qml
    Qt6::Sql
    Plasma::Plasma
    KF6::I18n
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::Archive
    KF6::KIOCore
    KF6::KCMUtils
    protobuf::libprotobuf
    csv
)

# Hide symbols coming only from bundled absl static archives so other
# static libraries (e.g. `csv`) keep their exported symbols for runtime
# detection (Plasma/KPlugin). We enumerate bundled `libabsl_*.a` files,
# collect their basenames and pass them to the linker via `--exclude-libs`.
file(GLOB ABSL_ARCHIVES "${TRANSIT_PEEK_PROTOBUF_INSTALL_DIR}/lib/libabsl_*.a")
if(ABSL_ARCHIVES)
    foreach(_absl_archive IN LISTS ABSL_ARCHIVES)
        get_filename_component(_absl_archive_name "${_absl_archive}" NAME)
        message(STATUS "Excluding exported symbols from bundled absl archive: ${_absl_archive_name}")
        target_link_options(id.lifers.transit_peek PRIVATE "-Wl,--exclude-libs,${_absl_archive_name}")
    endforeach()
endif()

set(GTFS_PROTO ${CMAKE_CURRENT_SOURCE_DIR}/gtfs-realtime.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${GTFS_PROTO})

target_sources(id.lifers.transit_peek PRIVATE
    DictionaryStore.cpp
    TransitFeed.cpp
    PresetModel.cpp
    SearchModel.cpp
)

target_sources(id.lifers.transit_peek PRIVATE ${PROTO_SRCS} ${PROTO_HDRS})
target_include_directories(id.lifers.transit_peek PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
