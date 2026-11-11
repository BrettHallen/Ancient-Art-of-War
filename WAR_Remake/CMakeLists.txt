cmake_minimum_required(VERSION 3.16)
project(AncientArtOfWar VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(SDL3 REQUIRED CONFIG COMPONENTS SDL3)

add_executable(AncientArtOfWar MACOSX_BUNDLE
    src/main.c
    src/assets.c
    src/gfx.c
    src/campaign.c
    src/play.c
    src/battle.c
    src/menu.c
)
target_link_libraries(AncientArtOfWar PRIVATE SDL3::SDL3)

if(MSVC)
    target_compile_options(AncientArtOfWar PRIVATE /W4)
else()
    target_compile_options(AncientArtOfWar PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(AncientArtOfWar PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "The Ancient Art of War"
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.ancientartofwar.remake"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
)
