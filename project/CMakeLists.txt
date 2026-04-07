cmake_minimum_required(VERSION 4.1)
project(project)

set(CMAKE_CXX_STANDARD 20)

# Homebrew 기본 경로로 설정
set(SFML_INCLUDE_DIR "/opt/homebrew/include")
set(SFML_LIBRARY_DIR "/opt/homebrew/lib")

include_directories(${SFML_INCLUDE_DIR})
link_directories(${SFML_LIBRARY_DIR})

add_executable(project main.cpp)

# 라이브러리 연결
target_link_libraries(project sfml-graphics sfml-window sfml-system sfml-audio)
