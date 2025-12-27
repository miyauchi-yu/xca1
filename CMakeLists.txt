cmake_minimum_required(VERSION 3.10)

# プロジェクト名
project(xca1 LANGUAGES CXX)

# C++の標準を設定
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ソースファイル
set(SOURCES
    src/main.cpp
    src/logger/logger.cpp
    src/analysis/analysis.cpp
    external/glad/src/glad.c
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
    external/ImGuiFileDialog/ImGuiFileDialog.cpp
)

# インクルードディレクトリ
include_directories(
    external/glad/include
    external/glfw/include
    /mingw64/include
    external/imgui
    external/imgui/backends
    external/ImGuiFileDialog
)

# パッケージ
find_library(GLFW3_LIB NAMES glfw3 glfw PATHS /mingw64/lib)
find_package(libzip REQUIRED)

# 実行ファイル
add_executable(${PROJECT_NAME} ${SOURCES})

# ライブラリーリンク
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${GLFW3_LIB}
    ${OPENGL_gl_LIBRARY}
    zip
    -lgdi32
)
target_include_directories(${PROJECT_NAME} PRIVATE
    external/glad/include
    external/imgui
    external/imgui/backends
    external/ImGuiFileDialog
)
set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
)

# glfw3.dllのコピー
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        /mingw64/bin/glfw3.dll
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMENT "Copying glfw3.dll to output directory..."
)
