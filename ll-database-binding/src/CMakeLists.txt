PROJECT_TARGET_ADD(ll-database-binding)

find_package(BerkeleyDB REQUIRED)
include_directories(${DB_INCLUDE_DIR})

add_library(ll-database-binding MODULE ll-database-binding.c utils.h)
target_link_libraries(ll-database-binding ${DB_LIBRARY})

set_target_properties(ll-database-binding PROPERTIES
        PREFIX "afb-"
        LABELS "BINDING"
        LINK_FLAGS ${BINDINGS_LINK_FLAG}
        OUTPUT_NAME ${TARGET_NAME})
