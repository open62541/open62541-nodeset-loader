
add_executable(check_sort check_sort.cpp)
target_include_directories(check_sort PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(check_sort CONAN_PKG::gtest xmlImport)
add_test(NAME sortTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin COMMAND check_sort ${CMAKE_CURRENT_LIST_DIR})

