cmake_minimum_required(VERSION 3.1)
project(pam_agl)

include(FindPkgConfig)

# Require PAM but there is no find_package
set(PAM_INCLUDE_DIR "/usr/include/")
set(PAM_LIBRARY "/lib64/libpam.so.0")
include_directories(${PAM_INCLUDE_DIR})

# Find json-c
pkg_check_modules(${JSON_C} REQUIRED json-c)
include_directories(${${JSON_C}_INCLUDE_DIRS})
add_compile_options(${${JSON_C}_CFLAGS})

# Add the target
add_library(pam_agl SHARED pam_agl.c)
target_link_libraries(pam_agl ${PAM_LIBRARY} ${${JSON_C}_LIBRARIES})
#list (APPEND link_libraries ${${JSON_C}_LDFLAGS})
set_property(TARGET pam_agl PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET pam_agl PROPERTY PREFIX "")

if (NOT DEFINED CMAKE_INSTALL_LIBDIR)
	get_filename_component(CMAKE_INSTALL_LIBDIR ${PAM_LIBRARY} DIRECTORY)
endif()

install(TARGETS pam_agl
	LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/security/")
