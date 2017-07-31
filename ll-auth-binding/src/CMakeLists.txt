PROJECT_TARGET_ADD(ll-auth-binding)

# Require PAM but there is no find_package
set(PAM_INCLUDE_DIR "/usr/include/")
set(PAM_LIBRARY "/lib64/libpam.so.0")
set(PAM_MISC_LIBRARY "/lib64/libpam_misc.so.0")
include_directories(${PAM_INCLUDE_DIR})

add_library(ll-auth-binding MODULE ll-auth-binding.c)

set_target_properties(ll-auth-binding PROPERTIES
	PREFIX "afb-"
	LABELS "BINDING"
	LINK_FLAGS ${BINDINGS_LINK_FLAG}
	OUTPUT_NAME ${TARGET_NAME})

target_link_libraries(ll-auth-binding ${link_libraries} ${PAM_LIBRARY} ${PAM_MISC_LIBRARY} Threads::Threads)

install(
	TARGETS ll-auth-binding
	LIBRARY DESTINATION ${BINDINGS_INSTALL_DIR})
