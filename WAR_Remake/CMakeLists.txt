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
    src/sound.c
    src/icon.c
    src/mapedit.c
    src/sqedit.c
    src/formed.c
    src/savegame.c
    src/gamelog.c
    src/composite.c
)
target_link_libraries(AncientArtOfWar PRIVATE SDL3::SDL3)

if(MSVC)
    target_compile_options(AncientArtOfWar PRIVATE /W4)
else()
    target_compile_options(AncientArtOfWar PRIVATE -Wall -Wextra -Wpedantic)
endif()

# The app icon is drawn at build time from your own copy of the game (the
# opening title page), so no original artwork is kept in the repository.
# Set WAR_GAME_DIR to the folder with WAR.EXE, or keep the files in
# ../WAR_Original_Files_3.5. Without them the app has no icon in Finder (the
# game still sets its window icon when it runs).
set(WAR_GAME_DIR "" CACHE PATH "Folder with your original game files (for the app icon)")
if(NOT WAR_GAME_DIR AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../WAR_Original_Files_3.5/WAR.EXE")
    set(WAR_GAME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../WAR_Original_Files_3.5")
endif()
if(APPLE AND WAR_GAME_DIR)
    add_executable(make_icon tools/make_icon.c src/icon.c src/assets.c src/gfx.c)
    target_link_libraries(make_icon PRIVATE SDL3::SDL3)
    set(WAR_ICONSET "${CMAKE_CURRENT_BINARY_DIR}/AppIcon.iconset")
    set(WAR_ICNS "${CMAKE_CURRENT_BINARY_DIR}/AppIcon.icns")
    add_custom_command(OUTPUT "${WAR_ICNS}"
        COMMAND "${CMAKE_COMMAND}" -E remove_directory "${WAR_ICONSET}"
        COMMAND make_icon "${WAR_GAME_DIR}" "${WAR_ICONSET}"
        COMMAND iconutil -c icns "${WAR_ICONSET}" -o "${WAR_ICNS}"
        DEPENDS make_icon
        COMMENT "Drawing the app icon from ${WAR_GAME_DIR}"
        VERBATIM)
    target_sources(AncientArtOfWar PRIVATE "${WAR_ICNS}")
    set_source_files_properties("${WAR_ICNS}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_target_properties(AncientArtOfWar PROPERTIES MACOSX_BUNDLE_ICON_FILE AppIcon)
endif()

set_target_properties(AncientArtOfWar PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "The Ancient Art of War"
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.ancientartofwar.remake"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
)
