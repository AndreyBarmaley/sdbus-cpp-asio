cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(sdbus_cpp_asio)

add_custom_command(
    OUTPUT asio_test_adaptor.h
    COMMAND sdbus-c++-xml2cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/test/test_dbus.xml
                    --adaptor=${CMAKE_BINARY_DIR}/asio_test_adaptor.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/test/test_dbus.xml
    COMMENT "Generating D-Bus TEST adaptor interface"
)

add_custom_command(
    OUTPUT asio_test_proxy.h
    COMMAND sdbus-c++-xml2cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/test/test_dbus.xml
                    --proxy=${CMAKE_BINARY_DIR}/asio_test_proxy.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/test/test_dbus.xml
    COMMENT "Generating D-Bus TEST proxy interface"
)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)

find_package(sdbus-c++ REQUIRED)
find_package(Boost 1.78 REQUIRED)

# adaptor
add_executable(test_adaptor asio_test_adaptor.h src/test/test_adaptor.cpp)

if(${sdbus-c++_VERSION} VERSION_GREATER_EQUAL "2.0.0")
    target_compile_definitions(test_adaptor PRIVATE -DSDBUS_2_0_API)
endif()

target_include_directories(test_adaptor PRIVATE ${Boost_INCLUDE_DIRS} src ${CMAKE_BINARY_DIR})
target_link_libraries(test_adaptor SDBusCpp::sdbus-c++ ${Boost_LIBRARIES} pthread)

set_target_properties(test_adaptor PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# proxy
add_executable(test_proxy asio_test_proxy.h src/test/test_proxy.cpp)

if(${sdbus-c++_VERSION} VERSION_GREATER_EQUAL "2.0.0")
    target_compile_definitions(test_proxy PRIVATE -DSDBUS_2_0_API)
endif()

target_include_directories(test_proxy PRIVATE ${Boost_INCLUDE_DIRS} src ${CMAKE_BINARY_DIR})
target_link_libraries(test_proxy SDBusCpp::sdbus-c++ ${Boost_LIBRARIES} pthread)

set_target_properties(test_proxy PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
