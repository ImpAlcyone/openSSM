#tests/CMakeLists.txt

cmake_minimum_required(VERSION 3.14)

add_executable(test_ssm test_ssm.c ../src/ssm.c)
target_include_directories(test_ssm PRIVATE ../include/src)
target_link_libraries(test_ssm cmocka)
