cmake_minimum_required(VERSION 3.16)
project(netmon VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -pedantic -Wno-unused-parameter)
endif()

# Build metadata & version header generation
set(WHOAMI_OUTPUT)
execute_process(
    COMMAND whoami
    OUTPUT_VARIABLE WHOAMI_OUTPUT
    RESULT_VARIABLE WHOAMI_RESULT
    ERROR_VARIABLE WHOAMI_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(HOSTNAME_OUTPUT)
execute_process(
    COMMAND hostname
    OUTPUT_VARIABLE HOSTNAME_OUTPUT
    RESULT_VARIABLE HOSTNAME_RESULT
    ERROR_VARIABLE HOSTNAME_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(DATE_OUTPUT)
execute_process(
    COMMAND date "+%Y-%m-%d %H:%M:%S"
    OUTPUT_VARIABLE DATE_OUTPUT
    RESULT_VARIABLE DATE_RESULT
    ERROR_VARIABLE DATE_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Version.hxx.in
    ${CMAKE_CURRENT_BINARY_DIR}/Version.hxx
    @ONLY
)

# Dependencies
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(CONFIGPP REQUIRED libconfig++)
pkg_check_modules(PCAP REQUIRED libpcap)

# Source files
set(SOURCES
    src/Main.cxx
    src/Config.cxx
    src/OuiDatabase.cxx
    src/DeviceRegistry.cxx
    src/LanSniffer.cxx
    src/ZyxelDriver.cxx
    src/SnmpAggregator.cxx
    src/SecurityCheckpoint.cxx
    src/AimonGatewayClient.cxx
    src/NetMonShell.cxx
)

add_executable(netmon ${SOURCES})

target_include_directories(netmon PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/json/include
    ${CONFIGPP_INCLUDE_DIRS}
    ${PCAP_INCLUDE_DIRS}
)

target_link_libraries(netmon PRIVATE
    Threads::Threads
    ${CONFIGPP_LIBRARIES}
    ${PCAP_LIBRARIES}
)

if(CONFIGPP_LIBRARY_DIRS)
    target_link_directories(netmon PRIVATE ${CONFIGPP_LIBRARY_DIRS})
endif()
if(PCAP_LIBRARY_DIRS)
    target_link_directories(netmon PRIVATE ${PCAP_LIBRARY_DIRS})
endif()
