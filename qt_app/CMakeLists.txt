cmake_minimum_required(VERSION 3.16)
project(HuiwenWorkspace LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Widgets Concurrent)

# 鸿蒙由 QPA 加载共享库；桌面系统使用普通可执行入口。
set(workspace_sources
    main.cpp
    resources/file-icons.qrc
    src/workspace/ConversionWindow.cpp
    src/sourceView/SourceDocumentView.cpp
    src/sourceView/PdfPreviewWidget.cpp
    src/resultView/MarkdownResultView.cpp
    src/components/FileCard.cpp
    src/components/ConvertButton.cpp
    src/services/ConversionService.cpp
    src/sandbox/SandboxService.cpp)
if(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
    add_library(huiwen_qt SHARED ${workspace_sources})
    target_link_libraries(huiwen_qt PRIVATE huiwen_native Qt5::QOpenHarmonyPlatformIntegrationPlugin)
    add_custom_command(TARGET huiwen_qt POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:huiwen_qt>"
                "${project_root}/entry/libs/arm64-v8a/$<TARGET_FILE_NAME:huiwen_qt>"
        COMMENT "同步 huiwen_qt 动态库到 entry/libs/arm64-v8a")
else()
    add_executable(huiwen_qt ${workspace_sources})
endif()
target_include_directories(huiwen_qt PRIVATE src ../entry/src/main/cpp)
# 工作区使用 PDFium 的公开 C API；桌面构建需指定匹配宿主架构的库。
if(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
    set(PDFIUM_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/../entry/libs/arm64-v8a/libpdfium.so")
else()
    set(PDFIUM_LIBRARY "" CACHE FILEPATH "宿主架构的 PDFium 动态库")
endif()
if(NOT EXISTS "${PDFIUM_LIBRARY}")
    message(FATAL_ERROR "缺少 PDFium 动态库，请准备 libpdfium.so 或设置 PDFIUM_LIBRARY")
endif()
add_library(huiwen_pdfium SHARED IMPORTED)
set_target_properties(huiwen_pdfium PROPERTIES
    IMPORTED_LOCATION "${PDFIUM_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/pdfium/public")
target_link_libraries(huiwen_qt PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Concurrent huiwen_pdfium)
