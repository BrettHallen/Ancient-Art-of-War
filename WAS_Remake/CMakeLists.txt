cmake_minimum_required(VERSION 3.16)
project(AncientArtOfWarAtSea VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(SDL3 REQUIRED CONFIG COMPONENTS SDL3)

add_executable(AncientArtOfWarAtSea MACOSX_BUNDLE
    src/main.c
    src/assets.c
    src/gfx.c
    src/campaign.c
    src/viewer.c
    src/play.c
    src/fiber.c
    src/battle.c
    src/battle_ships.c
    src/battle_gfx.c src/boarding.c src/enemy.c src/dsmap.c src/menu.c src/edmap.c src/edships.c src/gamelog.c
    src/icon.c
)
target_link_libraries(AncientArtOfWarAtSea PRIVATE SDL3::SDL3)

if(MSVC)
    target_compile_options(AncientArtOfWarAtSea PRIVATE /W4)
else()
    target_compile_options(AncientArtOfWarAtSea PRIVATE -Wall -Wextra -Wpedantic)
endif()

# The app icon is drawn at build time from your own copy of the game (the
# title page), so no original artwork is kept in the repository.
# Set WAS_GAME_DIR to the folder with WAS.EXE, or keep the files in
# ../WAS_Original_Files_3.5. Without them the app has no icon in Finder (the
# program still sets its window icon when it runs).
set(WAS_GAME_DIR "" CACHE PATH "Folder with your original game files (for the app icon)")
if(NOT WAS_GAME_DIR AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../WAS_Original_Files_3.5/WAS.EXE")
    set(WAS_GAME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../WAS_Original_Files_3.5")
endif()
if(APPLE AND WAS_GAME_DIR)
    add_executable(make_icon tools/make_icon.c src/icon.c src/assets.c src/gfx.c)
    target_link_libraries(make_icon PRIVATE SDL3::SDL3)
    set(WAS_ICONSET "${CMAKE_CURRENT_BINARY_DIR}/AppIcon.iconset")
    set(WAS_ICNS "${CMAKE_CURRENT_BINARY_DIR}/AppIcon.icns")
    add_custom_command(OUTPUT "${WAS_ICNS}"
        COMMAND "${CMAKE_COMMAND}" -E remove_directory "${WAS_ICONSET}"
        COMMAND make_icon "${WAS_GAME_DIR}" "${WAS_ICONSET}"
        COMMAND iconutil -c icns "${WAS_ICONSET}" -o "${WAS_ICNS}"
        DEPENDS make_icon
        COMMENT "Drawing the app icon from ${WAS_GAME_DIR}"
        VERBATIM)
    target_sources(AncientArtOfWarAtSea PRIVATE "${WAS_ICNS}")
    set_source_files_properties("${WAS_ICNS}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_target_properties(AncientArtOfWarAtSea PROPERTIES MACOSX_BUNDLE_ICON_FILE AppIcon)
endif()

set_target_properties(AncientArtOfWarAtSea PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "The Ancient Art of War at Sea"
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.ancientartofwaratsea.remake"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
)
