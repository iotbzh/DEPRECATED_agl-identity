
# Add PAM dependency
# FIXME: Require PAM, but it's seems there is no find_package.
set(PAM_INCLUDE_DIR "/usr/include/")
set(PAM_LIB "/lib64/libpam.so.0")
include_directories(${PAM_INCLUDE_DIR})

# Add the target
add_library(agl-identity-usbstick-pam SHARED agl-identity-usbstick-pam.c)
target_link_libraries(agl-identity-usbstick-pam ${PAM_LIB})
set_property(TARGET agl-identity-usbstick-pam PROPERTY POSITION_IDENPENDENT_CODE ON)
