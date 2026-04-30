cmake_minimum_required(VERSION 3.16)
project(ryzed)

find_package(SDL3)
add_subdirectory("third_party/filedialog")

set(RYZOM_CORE_PATH "R:/" CACHE STRING "Path to the Ryzom Core directory")
set(IMGUI_DIR "third_party/imgui" CACHE STRING "Path to the ImGui directory")

add_executable(ryzed
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdlgpu3.cpp
    src/UI/UIManager.cpp
    src/main.cpp)

target_include_directories(ryzed PRIVATE ${IMGUI_DIR})
target_include_directories(ryzed PRIVATE ${IMGUI_DIR})

target_link_libraries(ryzed PRIVATE SDL3::SDL3)
target_link_libraries(ryzed PRIVATE nfd)

if (WIN32)
    add_custom_command (TARGET ryzed POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different 
        "${SDL3_DIR}/../lib/x64/SDL3.dll"
        "$<TARGET_FILE_DIR:ryzed>")
endif ()
