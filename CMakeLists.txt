cmake_minimum_required(VERSION 3.16)
# 1. Меняем язык проекта на CXX (C++)
project(ProcLaunch LANGUAGES CXX)
 
# 2. Устанавливаем стандарт C++11 (вместо C99)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
 
# 3. Собираем библиотеку из src/proclaunch.cpp
add_library(proclaunch STATIC src/proclaunch.cpp)

# 4. Исправляем таргет: подключаем директорию include к библиотеке proclaunch
target_include_directories(proclaunch PUBLIC include)
 
add_executable(proctest test/proctest.cpp)
target_link_libraries(proctest PRIVATE proclaunch)
 
# 5. Меняем флаги компилятора под CXX
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(proclaunch PRIVATE -Wall -Wextra)
    target_compile_options(proctest PRIVATE -Wall -Wextra)
endif()

