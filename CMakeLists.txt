cmake_minimum_required(VERSION 3.22)

if(NOT TARGET GoodNet::sdk)
    project(goodnet_protocol_gnet
        VERSION   1.0.0
        LANGUAGES CXX)
    find_package(GoodNet REQUIRED)
    include(CTest)
    if(BUILD_TESTING)
        find_package(GTest      REQUIRED)
        find_package(rapidcheck QUIET)
        set(GOODNET_BUILD_TESTS ON)
    endif()
endif()

# GNET — canonical mesh-framing implementation. Statically linked
# into the kernel binary; the source still lives here under plugins/
# so the kernel/plugin separation is structural, not implementation-
# detail.

add_library(goodnet_protocol_gnet STATIC
    wire.cpp
    protocol.cpp
    gnet_register.cpp
)
add_library(GoodNet::protocol_gnet ALIAS goodnet_protocol_gnet)

target_include_directories(goodnet_protocol_gnet PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:include/goodnet>"
)

target_link_libraries(goodnet_protocol_gnet
    PUBLIC
        GoodNet::sdk
        GoodNet::ctx_accessors
    PRIVATE
        PkgConfig::SODIUM
)
# pkg_check_modules IMPORTED_TARGET sets INTERFACE_COMPILE_OPTIONS (not
# INTERFACE_INCLUDE_DIRECTORIES) when the CFLAGS contain -I entries on some
# CMake versions; the include never makes it into the compile command.
# Use SODIUM_CFLAGS directly as compile options so sodium.h is found.
if(SODIUM_CFLAGS)
    target_compile_options(goodnet_protocol_gnet PRIVATE ${SODIUM_CFLAGS})
endif()
# protocol.cpp / wire.cpp consume `core/util/endian.hpp` from the
# kernel tree (reachable through the PUBLIC include directories
# entry above, PROJECT_SOURCE_DIR covers core/) plus
# `sodium_memcmp` from libsodium (PRIVATE — used inside the static
# archive only, doesn't bleed into the public link surface).
# Linking `GoodNet::kernel` would route consumers back through the
# kernel graph and prevent the install-tree shape below from being
# a decoupled add-on.

target_compile_features(goodnet_protocol_gnet PUBLIC cxx_std_23)

# Export under the `GoodNet::protocol_gnet` alias name (matches the
# in-tree alias above so consumers using `find_package(GoodNet)`
# spell the dependency identically to a source-tree subdirectory
# build).
set_target_properties(goodnet_protocol_gnet PROPERTIES
    EXPORT_NAME protocol_gnet)

install(TARGETS goodnet_protocol_gnet
    EXPORT GoodNetProtocolGnetTargets
    ARCHIVE)

install(EXPORT GoodNetProtocolGnetTargets
    NAMESPACE   GoodNet::
    DESTINATION lib/cmake/GoodNetProtocolGnet)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/GoodNetProtocolGnetConfig.cmake"
    "include(\"\${CMAKE_CURRENT_LIST_DIR}/GoodNetProtocolGnetTargets.cmake\")\n")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/GoodNetProtocolGnetConfig.cmake"
    DESTINATION lib/cmake/GoodNetProtocolGnet)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/sdk/gnet.h"
    DESTINATION "include/goodnet/sdk")

if(GOODNET_BUILD_TESTS)
    add_subdirectory(tests)
endif()
