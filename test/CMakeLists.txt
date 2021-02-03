cmake_minimum_required(VERSION 3.11)
project(check_tests C)

set(CMAKE_C_STANDARD 99)
find_package(Threads REQUIRED)
####################################################################################
# <Libcheck libs>                                                                    #
####################################################################################

add_executable(check_tests ../src/main.c)
TARGET_LINK_LIBRARIES(check_tests pthread Threads::Threads)
#add_executable(TP0_test template.c)
