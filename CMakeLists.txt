cmake_minimum_required(VERSION 3.16)
project(netmon VERSION 1.0.6 LANGUAGES CXX C)

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
find_package(Curses REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(CONFIGPP REQUIRED libconfig++)
pkg_check_modules(PCAP REQUIRED libpcap)
pkg_check_modules(NETSNMP REQUIRED netsnmp)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)
pkg_check_modules(OPENSSL REQUIRED openssl)
pkg_check_modules(LIBSSH2 REQUIRED libssh2)
pkg_check_modules(CPPUTEST REQUIRED cpputest)

add_compile_definitions(CPPHTTPLIB_OPENSSL_SUPPORT)

# Source files
set(SOURCES
    src/Main.cxx
    src/Config.cxx
    src/OuiDatabase.cxx
    src/MacVendorResolver.cxx
    src/VendorTaxonomy.cxx
    src/DnsResolver.cxx
    src/DeviceRegistry.cxx
    src/LanSniffer.cxx
    src/ZyxelDriver.cxx
    src/ZyxelSshClient.cxx
    src/SnmpAggregator.cxx
    src/SnmpDatabase.cxx
    src/WebServer.cxx
    src/SecurityCheckpoint.cxx
    src/AimonGatewayClient.cxx
    src/NetMonShell.cxx
    src/AuthManager.cxx
    src/SyslogServer.cxx
    src/InstanceLock.cxx
    src/NcursesConsole.cxx
    src/AiSecurityClearance.cxx
    src/UnixAuth.cxx
    src/zyxel/ZyxelScanner.cxx
    src/zyxel/ZyxelSystemCmd.cxx
    src/zyxel/ZyxelNetworkCmd.cxx
    src/zyxel/ZyxelObjectCmd.cxx
    src/zyxel/ZyxelFirewallCmd.cxx
    src/zyxel/ZyxelNatCmd.cxx
    src/zyxel/ZyxelSecurityCmd.cxx
)

add_executable(netmon ${SOURCES})

target_include_directories(netmon PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/json/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cpp-httplib
    ${CURSES_INCLUDE_DIRS}
    ${CONFIGPP_INCLUDE_DIRS}
    ${PCAP_INCLUDE_DIRS}
    ${NETSNMP_INCLUDE_DIRS}
    ${SQLITE3_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
    ${LIBSSH2_INCLUDE_DIRS}
)

target_link_libraries(netmon PRIVATE
    Threads::Threads
    ${CURSES_LIBRARIES}
    ${CONFIGPP_LIBRARIES}
    ${PCAP_LIBRARIES}
    ${NETSNMP_LIBRARIES}
    ${SQLITE3_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${LIBSSH2_LIBRARIES}
)

if(CONFIGPP_LIBRARY_DIRS)
    target_link_directories(netmon PRIVATE ${CONFIGPP_LIBRARY_DIRS})
endif()
if(PCAP_LIBRARY_DIRS)
    target_link_directories(netmon PRIVATE ${PCAP_LIBRARY_DIRS})
endif()
if(NETSNMP_LIBRARY_DIRS)
    target_link_directories(netmon PRIVATE ${NETSNMP_LIBRARY_DIRS})
endif()
if(SQLITE3_LIBRARY_DIRS)
    target_link_directories(netmon PRIVATE ${SQLITE3_LIBRARY_DIRS})
endif()
if(OPENSSL_LIBRARY_DIRS)
    target_link_directories(netmon PRIVATE ${OPENSSL_LIBRARY_DIRS})
endif()
if(LIBSSH2_LIBRARY_DIRS)
    target_link_directories(netmon PRIVATE ${LIBSSH2_LIBRARY_DIRS})
endif()

# Clearance client executable
add_executable(netmon-ai-client client/netmon-ai-client.c)
set_target_properties(netmon-ai-client PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)

# Test suite executable
set(TEST_SOURCES
    test/TestMain.cxx
    test/RecordingRouter.cxx
    test/TestAuthManager.cxx
    test/TestCredentialVault.cxx
    test/TestApprovalQueue.cxx
    test/TestAuditTrail.cxx
    test/TestSyslogIngest.cxx
    test/TestInstanceLock.cxx
    test/TestNcursesConsole.cxx
    test/TestAiSecurityClearance.cxx
    test/TestAiSecurityAdversarial.cxx
    test/TestClearanceClient.cxx
    src/AiSecurityClearance.cxx
    src/AuthManager.cxx
    src/WebServer.cxx
    src/Config.cxx
    src/OuiDatabase.cxx
    src/MacVendorResolver.cxx
    src/VendorTaxonomy.cxx
    src/DnsResolver.cxx
    src/DeviceRegistry.cxx
    src/LanSniffer.cxx
    src/SnmpAggregator.cxx
    src/SnmpDatabase.cxx
    src/SecurityCheckpoint.cxx
    src/AimonGatewayClient.cxx
    src/NetMonShell.cxx
    src/SyslogServer.cxx
    src/ZyxelSshClient.cxx
    src/InstanceLock.cxx
    src/NcursesConsole.cxx
    src/UnixAuth.cxx
)

add_executable(test_netmon_suite ${TEST_SOURCES})

target_include_directories(test_netmon_suite PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/test
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/json/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cpp-httplib
    ${CURSES_INCLUDE_DIRS}
    ${CONFIGPP_INCLUDE_DIRS}
    ${PCAP_INCLUDE_DIRS}
    ${NETSNMP_INCLUDE_DIRS}
    ${SQLITE3_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
    ${LIBSSH2_INCLUDE_DIRS}
    ${CPPUTEST_INCLUDE_DIRS}
)

target_link_libraries(test_netmon_suite PRIVATE
    Threads::Threads
    ${CURSES_LIBRARIES}
    ${CONFIGPP_LIBRARIES}
    ${PCAP_LIBRARIES}
    ${NETSNMP_LIBRARIES}
    ${SQLITE3_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${LIBSSH2_LIBRARIES}
    ${CPPUTEST_LIBRARIES}
)

add_dependencies(test_netmon_suite netmon-ai-client)

if(CONFIGPP_LIBRARY_DIRS)
    target_link_directories(test_netmon_suite PRIVATE ${CONFIGPP_LIBRARY_DIRS})
endif()
if(PCAP_LIBRARY_DIRS)
    target_link_directories(test_netmon_suite PRIVATE ${PCAP_LIBRARY_DIRS})
endif()
if(NETSNMP_LIBRARY_DIRS)
    target_link_directories(test_netmon_suite PRIVATE ${NETSNMP_LIBRARY_DIRS})
endif()
if(SQLITE3_LIBRARY_DIRS)
    target_link_directories(test_netmon_suite PRIVATE ${SQLITE3_LIBRARY_DIRS})
endif()
if(OPENSSL_LIBRARY_DIRS)
    target_link_directories(test_netmon_suite PRIVATE ${OPENSSL_LIBRARY_DIRS})
endif()
if(LIBSSH2_LIBRARY_DIRS)
    target_link_directories(test_netmon_suite PRIVATE ${LIBSSH2_LIBRARY_DIRS})
endif()
if(CPPUTEST_LIBRARY_DIRS)
    target_link_directories(test_netmon_suite PRIVATE ${CPPUTEST_LIBRARY_DIRS})
endif()

# Dedicated live driver & parser test suite executable
set(ZYXEL_TEST_SOURCES
    test/TestZyxelMain.cxx
    test/TestZyxelScanner.cxx
    test/TestZyxelSshClient.cxx
    test/TestZyxelDriver.cxx
    test/TestZyxelSystemCmd.cxx
    test/TestZyxelNetworkCmd.cxx
    test/TestZyxelObjectCmd.cxx
    test/TestZyxelFirewallCmd.cxx
    test/TestZyxelNatCmd.cxx
    test/TestZyxelSecurityCmd.cxx
    src/ZyxelDriver.cxx
    src/ZyxelSshClient.cxx
    src/zyxel/ZyxelScanner.cxx
    src/zyxel/ZyxelSystemCmd.cxx
    src/zyxel/ZyxelNetworkCmd.cxx
    src/zyxel/ZyxelObjectCmd.cxx
    src/zyxel/ZyxelFirewallCmd.cxx
    src/zyxel/ZyxelNatCmd.cxx
    src/zyxel/ZyxelSecurityCmd.cxx
    src/AuthManager.cxx
    src/Config.cxx
)

add_executable(test_zyxel_driver_suite ${ZYXEL_TEST_SOURCES})

target_include_directories(test_zyxel_driver_suite PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/test
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/json/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cpp-httplib
    ${CONFIGPP_INCLUDE_DIRS}
    ${PCAP_INCLUDE_DIRS}
    ${NETSNMP_INCLUDE_DIRS}
    ${SQLITE3_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
    ${LIBSSH2_INCLUDE_DIRS}
    ${CPPUTEST_INCLUDE_DIRS}
)

target_link_libraries(test_zyxel_driver_suite PRIVATE
    Threads::Threads
    ${CONFIGPP_LIBRARIES}
    ${PCAP_LIBRARIES}
    ${NETSNMP_LIBRARIES}
    ${SQLITE3_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${LIBSSH2_LIBRARIES}
    ${CPPUTEST_LIBRARIES}
)

if(CONFIGPP_LIBRARY_DIRS)
    target_link_directories(test_zyxel_driver_suite PRIVATE ${CONFIGPP_LIBRARY_DIRS})
endif()
if(PCAP_LIBRARY_DIRS)
    target_link_directories(test_zyxel_driver_suite PRIVATE ${PCAP_LIBRARY_DIRS})
endif()
if(NETSNMP_LIBRARY_DIRS)
    target_link_directories(test_zyxel_driver_suite PRIVATE ${NETSNMP_LIBRARY_DIRS})
endif()
if(SQLITE3_LIBRARY_DIRS)
    target_link_directories(test_zyxel_driver_suite PRIVATE ${SQLITE3_LIBRARY_DIRS})
endif()
if(OPENSSL_LIBRARY_DIRS)
    target_link_directories(test_zyxel_driver_suite PRIVATE ${OPENSSL_LIBRARY_DIRS})
endif()
if(LIBSSH2_LIBRARY_DIRS)
    target_link_directories(test_zyxel_driver_suite PRIVATE ${LIBSSH2_LIBRARY_DIRS})
endif()
if(CPPUTEST_LIBRARY_DIRS)
    target_link_directories(test_zyxel_driver_suite PRIVATE ${CPPUTEST_LIBRARY_DIRS})
endif()

add_executable(test_zyxel_parsers_fuzz
    test/fuzz/TestZyxelParsersFuzz.cxx
    src/zyxel/ZyxelScanner.cxx
    src/zyxel/ZyxelSystemCmd.cxx
    src/zyxel/ZyxelNetworkCmd.cxx
    src/zyxel/ZyxelObjectCmd.cxx
    src/zyxel/ZyxelFirewallCmd.cxx
    src/zyxel/ZyxelNatCmd.cxx
    src/zyxel/ZyxelSecurityCmd.cxx
    src/ZyxelSshClient.cxx
    src/Config.cxx
)

target_include_directories(test_zyxel_parsers_fuzz PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/json/include
    ${LIBSSH2_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
    ${CONFIGPP_INCLUDE_DIRS}
)

target_link_libraries(test_zyxel_parsers_fuzz PRIVATE
    rapidcheck
    ${LIBSSH2_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${CONFIGPP_LIBRARIES}
    Threads::Threads
)

add_custom_target(test
    COMMAND test_netmon_suite -v
    COMMAND test_zyxel_driver_suite -v
    COMMAND test_zyxel_parsers_fuzz
    DEPENDS test_netmon_suite test_zyxel_driver_suite test_zyxel_parsers_fuzz
)

