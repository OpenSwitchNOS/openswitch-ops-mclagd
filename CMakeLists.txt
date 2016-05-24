# (C) Copyright 2016 Hewlett Packard Enterprise Development LP
#
#  Licensed under the Apache License, Version 2.0 (the "License"); you may
#  not use this file except in compliance with the License. You may obtain
#  a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
#  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
#  License for the specific language governing permissions and limitations
#  under the License.

cmake_minimum_required (VERSION 2.8)

set (OPSMCLAGD ops-mclagd)
project (${OPSMCLAGD})
set (OPSMCLAGKAD ops-mclagkad)

set (SRC_DIR src)
set (INCL_DIR include)

# This option is specified in Yocto -- openswitch.bbclass.
OPTION( CPU_LITTLE_ENDIAN "Specifies CPU architecture is Little-Endian" OFF )

# Define compile flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall -Werror -ggdb -g3 -O0")

# Rules to locate needed libraries
include(FindPkgConfig)
pkg_check_modules(OVSCOMMON REQUIRED libovscommon)
pkg_check_modules(OVSDB REQUIRED libovsdb)
pkg_check_modules(OVSDB REQUIRED opsutils)

include_directories (${PROJECT_BINARY_DIR}/${INCL_DIR}
                     ${PROJECT_SOURCE_DIR}/${INCL_DIR}
                     ${PROJECT_SOURCE_DIR}
                     ${OVSCOMMON_INCLUDE_DIRS})

# Source files to build ops-mclagd
set (SOURCES_MCLAGD ${SRC_DIR}/mclagd.c )


# Rules to build ops-mclagd
add_executable (${OPSMCLAGD} ${SOURCES_MCLAGD})

target_link_libraries (${OPSMCLAGD} ${OVSCOMMON_LIBRARIES} ${OVSDB_LIBRARIES}
                       ${OPSUTILS_LIBRARIES}
                       -lpthread -lrt)

#add_subdirectory(src/cli)

# Rules to install ops-mclagd binary in rootfs
install(TARGETS ${OPSMCLAGD}
        RUNTIME DESTINATION bin)
