cmake_minimum_required(VERSION 3.3)
project(idkey)
add_executable(idkey main.c)
install(TARGETS idkey
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib)
