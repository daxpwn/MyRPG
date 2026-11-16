cmake_minimum_required(VERSION 3.25)
project(game LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- raylib -----------------------------------------------------------------
# FetchContent skida raylib sa GitHub-a pri prvom configure-u i bilduje ga
# zajedno sa tvojim kodom. Ne moras nista rucno da instaliras.
include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(raylib)

# --- tvoja igrica -----------------------------------------------------------
# Kad dodas novi .cpp fajl u src/, dodaj ga i ovde.
add_executable(game
    src/main.cpp
    src/tileset.cpp
    src/tilemap.cpp
    src/player.cpp
    src/chunks.cpp
    src/debugmap.cpp
    src/weapons.cpp
    src/inventory.cpp
    src/shooting.cpp
    src/survival.cpp
    src/menu.cpp
    src/game.cpp
    src/shop.cpp
)
target_link_libraries(game PRIVATE raylib)

# Putanja do assets/ foldera, dostupna u kodu kao ASSETS_DIR.
# Igra cita fajlove direktno iz projekta, pa izmene u map.txt vidis odmah (R).
target_compile_definitions(game PRIVATE ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets/")

if(MSVC)
    target_compile_options(game PRIVATE /W4)
else()
    target_compile_options(game PRIVATE -Wall -Wextra)
endif()
