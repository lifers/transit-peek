add_definitions(-DTRANSLATION_DOMAIN="plasma_applet_id.lifers.transit_peek")

plasma_add_applet(id.lifers.transit_peek
    QML_SOURCES
        qml/main.qml
    CPP_SOURCES
        gtfs-reader.cpp
    GENERATE_APPLET_CLASS
)
find_package(Protobuf REQUIRED)

target_compile_features(id.lifers.transit_peek PUBLIC cxx_std_20)
target_link_libraries(id.lifers.transit_peek PRIVATE
    Qt6::Gui
    Plasma::Plasma
    KF6::I18n
    ${Protobuf_LIBRARIES}
)

set(GTFS_PROTO ${CMAKE_CURRENT_SOURCE_DIR}/gtfs-realtime.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${GTFS_PROTO})

target_sources(id.lifers.transit_peek PRIVATE ${PROTO_SRCS} ${PROTO_HDRS})
target_include_directories(id.lifers.transit_peek PRIVATE ${CMAKE_CURRENT_BINARY_DIR})