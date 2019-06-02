find_package(Check REQUIRED)
include_directories(${CHECK_INCLUDE_DIRS})

add_executable(check_sort check_sort.c)
target_compile_options(check_sort PRIVATE ${compileFlags})
target_include_directories(check_sort PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(check_sort ${CHECK_LIBRARIES} pthread xmlImport)