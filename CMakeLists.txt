cmake_minimum_required(VERSION 3.13)
project(mavspyv2 LANGUAGES C)

add_executable(${PROJECT_NAME} main.c gui.c
    capture.c fsm.c)
target_link_libraries(${PROJECT_NAME} PUBLIC pcap)
target_compile_options(${PROJECT_NAME} PUBLIC 
    -Wno-address-of-packed-member)

find_package(SDL2)
target_include_directories(${PROJECT_NAME} PUBLIC
    ${SDL_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC
    SDL2::SDL2 GL)
