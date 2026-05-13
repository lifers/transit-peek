cmake_minimum_required(VERSION 3.22)

project(transit-peek)

set(PLASMA_TARGET_VERSION "6.6")
set(QT_MIN_VERSION "6.10")
set(KF_MIN_VERSION "6.22")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)

add_compile_definitions(
    QT_ENABLE_STRICT_MODE_UP_TO=0x061002
)

find_package(Qt6 ${QTMIN_VERSION} CONFIG REQUIRED COMPONENTS
    Quick
    Core
    Qml
)
find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    I18n
    Config
)
find_package(Plasma ${PLASMA_TARGET_VERSION} REQUIRED)

add_subdirectory(src)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)