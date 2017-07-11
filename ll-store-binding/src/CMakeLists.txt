PROJECT_TARGET_ADD(ll-store-binding)

list(APPEND link_libraries "/usr/lib64/liblmdb.so")
add_library(ll-store-binding MODULE ll-store-binding.c)

set_target_properties(ll-store-binding PROPERTIES
	PREFIX "afb-"
	LABELS "BINDING"
	LINK_FLAGS ${BINDINGS_LINK_FLAG}
	OUTPUT_NAME ${TARGET_NAME})

target_link_libraries(ll-store-binding ${link_libraries})

install(
	TARGETS ll-store-binding
	LIBRARY DESTINATION ${BINDINGS_INSTALL_DIR})
