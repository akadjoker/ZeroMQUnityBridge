cmake_minimum_required(VERSION 3.10)
project(ZeroMQBridge VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_UNITY_PLUGIN "Build Unity plugin" ON)
option(BUILD_EXAMPLES "Build examples" OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(UNITY_PLUGINS_DIR ${CMAKE_SOURCE_DIR}/unity/Plugins)


 
set(ZMQBRIDGE_SOURCES
    src/ZMQBridge.cpp
    src/Context.cpp
    src/Sockets.cpp
)

 
set(ZMQBRIDGE_HEADERS
    include/ZMQBridge.h
    src/Internal.h
)


find_package(ZeroMQ QUIET)
if(NOT ZeroMQ_FOUND)
 
    find_path(ZeroMQ_INCLUDE_DIR zmq.h)
    find_library(ZeroMQ_LIBRARY zmq)
    
    if(NOT ZeroMQ_INCLUDE_DIR OR NOT ZeroMQ_LIBRARY)
        message(FATAL_ERROR "ZeroMQ not found. Please install libzmq development packages.")
    endif()
    
    add_library(ZeroMQ::ZeroMQ UNKNOWN IMPORTED)
    set_target_properties(ZeroMQ::ZeroMQ PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${ZeroMQ_INCLUDE_DIR}"
        IMPORTED_LOCATION "${ZeroMQ_LIBRARY}"
    )
endif()

add_library(ZeroMQBridge ${ZMQBRIDGE_SOURCES} ${ZMQBRIDGE_HEADERS})

target_include_directories(ZeroMQBridge 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CPPZMQ_INCLUDE_DIR}
)

target_link_libraries(ZeroMQBridge PRIVATE ZeroMQ::ZeroMQ)

 
if(WIN32)
    target_compile_definitions(ZeroMQBridge PRIVATE -DZMQ_STATIC)
    
    if(MSVC)
 
        target_compile_definitions(ZeroMQBridge PRIVATE -D_CRT_SECURE_NO_WARNINGS)
    endif()
endif()

if(BUILD_UNITY_PLUGIN)
 
if(WIN32)
 
    set_target_properties(ZeroMQBridge PROPERTIES
        PREFIX ""
        SUFFIX ".dll"
    )
elseif(APPLE)
 
    set_target_properties(ZeroMQBridge PROPERTIES
        PREFIX ""
        SUFFIX ".bundle"
        MACOSX_RPATH ON
    )
else()
 
    set_target_properties(ZeroMQBridge PROPERTIES
        PREFIX "lib"
        SUFFIX ".so"
    )
endif()

 
     add_custom_command(TARGET ZeroMQBridge POST_BUILD
     COMMAND ${CMAKE_COMMAND} -E make_directory ${UNITY_PLUGINS_DIR}
     COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:ZeroMQBridge> ${UNITY_PLUGINS_DIR}
     COMMENT "Copying Unity plugin to ${UNITY_PLUGINS_DIR}"
 )
endif()

if(BUILD_EXAMPLES)
 
    add_executable(server samples/server.cpp)
    target_link_libraries(server PRIVATE ZeroMQBridge ZeroMQ::ZeroMQ)
    target_include_directories(server PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CPPZMQ_INCLUDE_DIR}
    )
    
 
    add_executable(client samples/client.cpp)
    target_link_libraries(client PRIVATE ZeroMQBridge ZeroMQ::ZeroMQ)
    target_include_directories(client PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CPPZMQ_INCLUDE_DIR}
    )
endif()  
