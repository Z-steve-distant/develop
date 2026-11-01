cmake_minimum_required(VERSION 3.20)

project(w3
    VERSION 0.1.0
    DESCRIPTION "W3 收官工程：lambda / std::function / TaskRunner / ASan 工具链"
    LANGUAGES CXX)

# ============================================================ 全局设置
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 给 clangd / IDE 生成 compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 单配置生成器（Makefile / Ninja）不指定构建类型时，默认 Debug
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "构建类型：Debug / Release / DebugASan" FORCE)
endif()

option(W3_WERROR "把编译警告当作错误（CI 用）" OFF)

include(GNUInstallDirs)
include(cmake/Sanitizers.cmake)

# ① 自定义构建类型 Debug-ASan（验收 C 档第 1 条）
# ② 探测工具链是否真的支持 -fsanitize=...，不支持就明确报错而不是假装能用
w3_define_asan_build_type()

# ============================================================ 编译警告
function(w3_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
        if(W3_WERROR)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor -Wold-style-cast)
        if(W3_WERROR)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

# ============================================================ 核心库
add_library(w3core STATIC
    src/task_runner.cpp)

add_library(w3::core ALIAS w3core)

target_include_directories(w3core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

w3_set_warnings(w3core)
w3_apply_sanitizers(w3core)

# ============================================================ 示例程序
function(w3_add_example name)
    add_executable(${name} examples/${name}.cpp)
    target_link_libraries(${name} PRIVATE w3::core)
    w3_set_warnings(${name})
    w3_apply_sanitizers(${name})
endfunction()

w3_add_example(ex_lambda_basics)
w3_add_example(ex_std_function)
w3_add_example(ex_task_runner)

# 故意写坏的 ASan 演示：不进默认构建、不进 ctest（跑起来就该崩）
add_executable(ex_asan_bug EXCLUDE_FROM_ALL examples/ex_asan_bug.cpp)
target_link_libraries(ex_asan_bug PRIVATE w3::core)
w3_apply_sanitizers(ex_asan_bug)

# ============================================================ 测试（CTest）
include(CTest)

if(BUILD_TESTING)
    function(w3_add_test name)
        add_executable(${name} tests/${name}.cpp)
        target_link_libraries(${name} PRIVATE w3::core)
        target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
        w3_set_warnings(${name})
        w3_apply_sanitizers(${name})
        add_test(NAME ${name} COMMAND ${name})
        # 泄漏检测在部分平台默认关闭，显式打开（见 W3_验收.md 的提示）
        set_tests_properties(${name} PROPERTIES
            ENVIRONMENT "ASAN_OPTIONS=detect_leaks=1")
    endfunction()

    w3_add_test(test_scope_guard)
    w3_add_test(test_task_runner)
endif()

# ============================================================ 安装与导出
install(TARGETS w3core EXPORT w3coreTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT w3coreTargets
    FILE w3coreTargets.cmake
    NAMESPACE w3::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/w3core)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    cmake/w3coreConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/w3coreConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/w3core)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/w3coreConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/w3coreConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/w3coreConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/w3core)

# ============================================================ 配置摘要
message(STATUS "──────────────────────────────────────────────")
message(STATUS "w3 配置摘要")
message(STATUS "  构建类型      : ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ 标准      : ${CMAKE_CXX_STANDARD}（扩展关闭）")
message(STATUS "  编译器        : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  sanitizer     : ${W3_SANITIZE}")
message(STATUS "  警告即错误    : ${W3_WERROR}")
message(STATUS "  测试          : ${BUILD_TESTING}")
message(STATUS "──────────────────────────────────────────────")
