cmake_minimum_required (VERSION 3.0.0)
project (avilib)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_STANDARD 17)

add_library (avi STATIC avilib.cpp reader.cpp writer.cpp)
target_compile_definitions(avi PRIVATE _CRT_SECURE_NO_WARNINGS)
add_compile_options(-Wno-multichar)
install(TARGETS avi DESTINATION lib)
install(FILES avilib.h DESTINATION include)

add_executable (avilib_test main.cpp)
target_link_libraries(avilib_test avi)
