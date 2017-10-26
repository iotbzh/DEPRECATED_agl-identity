###########################################################################
# Copyright 2015, 2016, 2017 IoT.bzh
#
# author: Loïc Collignon <loic.collignon@iot.bzh>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
###########################################################################

cmake_minimum_required(VERSION 3.3)
project(pam_agl)

include(FindPkgConfig)

# Require PAM but there is no find_package
set(PAM_INCLUDE_DIR "/usr/include/")
set(PAM_LIBRARY "/lib64/libpam.so.0")
include_directories(${PAM_INCLUDE_DIR})
if (NOT DEFINED CMAKE_INSTALL_LIBDIR)
	get_filename_component(CMAKE_INSTALL_LIBDIR ${PAM_LIBRARY} DIRECTORY)
endif()

# Find json-c
pkg_check_modules(${JSON_C} REQUIRED json-c)
include_directories(${${JSON_C}_INCLUDE_DIRS})
add_compile_options(${${JSON_C}_CFLAGS})

# Add the pam_agl_usb target
add_library(pam_agl_usb SHARED pam_agl_usb.c)
target_link_libraries(pam_agl_usb ${PAM_LIBRARY} ${${JSON_C}_LIBRARIES})
set_property(TARGET pam_agl_usb PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET pam_agl_usb PROPERTY PREFIX "")

install(TARGETS pam_agl_usb
	LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/security/")
	
# Add the pam_agl_nfc target
add_library(pam_agl_nfc SHARED pam_agl_nfc.c)
target_link_libraries(pam_agl_nfc ${PAM_LIBRARY} ${${JSON_C}_LIBRARIES})
set_property(TARGET pam_agl_nfc PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET pam_agl_nfc PROPERTY PREFIX "")

install(TARGETS pam_agl_nfc
	LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/security/")
