# Copyright 2016, alex at staticlibs.net
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required ( VERSION 2.8.12 )

# project
project ( wilton_server CXX )

# dep options
set ( staticlib_pion_USE_WILTON_LOGGING ON CACHE BOOL "Use wilton_logging lib for logging" )
set ( staticlib_pion_USE_OPENSSL ON CACHE BOOL "Use OpenSSL lib for https" )
set ( staticlib_pion_WILTON_INCLUDE ${WILTON_DIR}/core/include CACHE STRING "" )
set ( staticlib_pion_WILTON_LOGGING_INCLUDE ${WILTON_DIR}/modules/wilton_logging/include CACHE STRING "" )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# dependencies add
if ( STATICLIB_TOOLCHAIN MATCHES "(android|windows|macosx)_.+" )
    staticlib_add_subdirectory ( ${STATICLIB_DEPS}/external_asio )
endif ( ) 
if ( WILTON_BUILD_FLAVOUR MATCHES "wheezy" )
    staticlib_add_subdirectory ( ${STATICLIB_DEPS}/external_asio )
endif ( )
staticlib_add_subdirectory ( ${STATICLIB_DEPS}/staticlib_pion )
staticlib_add_subdirectory ( ${STATICLIB_DEPS}/staticlib_mustache )

# dependencies check
set ( ${PROJECT_NAME}_DEPS
        staticlib_config
        staticlib_support
        staticlib_io
        staticlib_ranges
        staticlib_pion
        staticlib_json
        staticlib_unzip
        staticlib_utils
        staticlib_mustache
        staticlib_tinydir
        staticlib_pimpl )
staticlib_pkg_check_modules ( ${PROJECT_NAME}_DEPS_PC REQUIRED ${PROJECT_NAME}_DEPS )



# library
set ( ${PROJECT_NAME}_PLATFORM_SRC ) 
set ( ${PROJECT_NAME}_RESFILE )
set ( ${PROJECT_NAME}_DEFFILE )
if ( STATICLIB_TOOLCHAIN MATCHES "windows_.+" )
    configure_file ( ${WILTON_DIR}/resources/buildres/wilton_module.rc
            ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rc )
    set ( ${PROJECT_NAME}_RESFILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rc )
    set ( ${PROJECT_NAME}_DEFFILE ${CMAKE_CURRENT_LIST_DIR}/resources/${PROJECT_NAME}.def )
    # header-only use
    list ( REMOVE_ITEM ${PROJECT_NAME}_DEPS_PC_LIBRARIES log4cplus )
    list ( REMOVE_ITEM ${PROJECT_NAME}_DEPS_PC_LIBRARIES openssl )
    list ( REMOVE_ITEM ${PROJECT_NAME}_DEPS_PC_LIBRARIES z )
endif ( )

add_library ( ${PROJECT_NAME} SHARED
        # mustache
        ${CMAKE_CURRENT_LIST_DIR}/src/mustache/wilton_mustache.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/mustache/wiltoncall_mustache.cpp
        # server
        ${CMAKE_CURRENT_LIST_DIR}/src/server/request.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/server/response_writer.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/server/server.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/server/wilton_server.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/server/wiltoncall_server.cpp
        # websocket_server
        ${CMAKE_CURRENT_LIST_DIR}/src/websocket_server/wilton_websocket_server.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/websocket_server/wiltoncall_websocket_server.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/websocket_server/websocket_handler.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/websocket_server/web_socket/WebSocket.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/websocket_server/web_socket/sha1/sha1.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/websocket_server/web_socket/base64/base64.cpp
        # export
        ${CMAKE_CURRENT_LIST_DIR}/src/wiltoncall_server.cpp
        ${${PROJECT_NAME}_RESFILE}
        ${${PROJECT_NAME}_DEFFILE} )

#set(BOOST_PATH "/home/mikhail/workspace/tmp/beast_test")
set(BOOST_PATH "/home/mikhail/workspace/utils/boost/")

target_link_libraries ( ${PROJECT_NAME} PRIVATE
        wilton_core
        wilton_crypto
        wilton_loader
        wilton_logging
        wilton_zip
        ${${PROJECT_NAME}_DEPS_PC_LIBRARIES} )
if ( STATICLIB_TOOLCHAIN MATCHES "(linux|maxosx)_.+" )
    target_link_libraries ( ${PROJECT_NAME} PRIVATE
            crypto ssl )
endif ( )

#add_definitions(-DASIO_STANDALONE)

target_include_directories ( ${PROJECT_NAME} BEFORE PRIVATE 
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${WILTON_DIR}/core/include
        ${WILTON_DIR}/modules/wilton_loader/include
        ${WILTON_DIR}/modules/wilton_logging/include
        ${BOOST_PATH}
        ${${PROJECT_NAME}_DEPS_PC_INCLUDE_DIRS} )

target_compile_options ( ${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_DEPS_PC_CFLAGS_OTHER} )

# platform-specific link options
if ( STATICLIB_TOOLCHAIN MATCHES "android_.+" )
    set_property ( TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY LINK_FLAGS "-Wl,-soname,lib${PROJECT_NAME}.so" )
elseif ( STATICLIB_TOOLCHAIN MATCHES "windows_.+" )
    target_link_libraries ( ${PROJECT_NAME} PRIVATE wtsapi32 )
    set_property ( TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY LINK_FLAGS "/manifest:no" )
endif ( )

# debuginfo
staticlib_extract_debuginfo_shared ( ${PROJECT_NAME} )

# pkg-config
set ( ${PROJECT_NAME}_PC_CFLAGS "-I${CMAKE_CURRENT_LIST_DIR}/include" )
set ( ${PROJECT_NAME}_PC_LIBS "-L${CMAKE_LIBRARY_OUTPUT_DIRECTORY} -l${PROJECT_NAME}" )
staticlib_list_to_string ( ${PROJECT_NAME}_PC_REQUIRES_PRIVATE "" ${PROJECT_NAME}_DEPS )
configure_file ( ${WILTON_DIR}/resources/buildres/pkg-config.in 
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/pkgconfig/${PROJECT_NAME}.pc )
