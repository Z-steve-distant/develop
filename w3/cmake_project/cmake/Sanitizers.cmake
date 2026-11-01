# =============================================================================
# Sanitizers.cmake —— ASan / UBSan / TSan 集成
#
# 两个职责：
#   1. 定义自定义构建类型 Debug-ASan（W3 验收 C 档第 1 条的要求）
#   2. 用 try_compile 真正探测工具链是否支持 -fsanitize=...
#      不支持就 FATAL_ERROR 并给出可执行的三条路径，而不是"配好选项、构建时炸"
#
# 为什么必须探测：MinGW-w64 的 GCC 不带 libasan，链接期才报
#   ld: cannot find -lasan
# 那时候错误已经离根因很远了。
# =============================================================================

include(CheckCXXSourceCompiles)

function(w3_define_asan_build_type)
    # ---- 1. 注册 DebugASan 这个构建类型的编译/链接选项 ----
    set(CMAKE_CXX_FLAGS_DEBUGASAN "-O1 -g" CACHE STRING "DebugASan：优化级别与调试信息" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS_DEBUGASAN "" CACHE STRING "DebugASan：链接选项（sanitizer 统一追加）" FORCE)

    # 单配置生成器：让 -DCMAKE_BUILD_TYPE 有补全提示
    if(CMAKE_BUILD_TYPE)
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
            Debug Release RelWithDebInfo MinSizeRel DebugASan)
    endif()

    # 多配置生成器（VS / Xcode / Ninja Multi-Config）：把 DebugASan 加进配置列表
    if(CMAKE_CONFIGURATION_TYPES)
        list(APPEND CMAKE_CONFIGURATION_TYPES DebugASan)
        list(REMOVE_DUPLICATES CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "" FORCE)
    endif()

    # ---- 2. 选中 DebugASan 就默认开 address;undefined ----
    if(CMAKE_BUILD_TYPE STREQUAL "DebugASan" AND NOT W3_SANITIZE)
        set(W3_SANITIZE "address;undefined" CACHE STRING "sanitizer 列表（分号分隔）" FORCE)
    endif()

    if(NOT W3_SANITIZE)
        return()
    endif()

    # ---- 3. MSVC 分支：只有 /fsanitize=address ----
    if(MSVC)
        if(NOT W3_SANITIZE STREQUAL "address")
            message(FATAL_ERROR
                "MSVC 只支持 address sanitizer，当前 W3_SANITIZE=${W3_SANITIZE}。")
        endif()
        set(CMAKE_REQUIRED_FLAGS "/fsanitize=address")
        check_cxx_source_compiles("int main() { return 0; }" W3_SANITIZER_OK
            COMPILE_OPTIONS /fsanitize=address)
        unset(CMAKE_REQUIRED_FLAGS)
        if(NOT W3_SANITIZER_OK)
            message(FATAL_ERROR
                "MSVC 的 /fsanitize=address 不可用。\n"
                "  需要 Visual Studio 2019 16.9+ 并安装 \"C++ AddressSanitizer\" 组件。")
        endif()
        return()
    endif()

    # ---- 4. GCC / Clang 分支：真实探测（编译 + 链接）----
    string(REPLACE ";" "," _san "${W3_SANITIZE}")

    unset(W3_SANITIZER_OK CACHE)   # 换过 sanitizer 列表时不要用陈旧缓存

    set(CMAKE_REQUIRED_FLAGS "-fsanitize=${_san} -fno-omit-frame-pointer")
    set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=${_san}")
    check_cxx_source_compiles(
        "int main() { int* p = new int(1); delete p; return 0; }" W3_SANITIZER_OK)
    unset(CMAKE_REQUIRED_FLAGS)
    unset(CMAKE_REQUIRED_LINK_OPTIONS)

    if(NOT W3_SANITIZER_OK)
        message(FATAL_ERROR
            "工具链不支持 -fsanitize=${_san}\n"
            "  编译器：${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}\n"
            "  MinGW-w64 的 GCC 不附带 libasan，链接会报 'cannot find -lasan'。\n"
            "\n"
            "  三条可行路径：\n"
            "    1) 用 WSL2 / Linux 上的 GCC 或 Clang 构建（周五的 ASan 实验本来就按 Linux 写）\n"
            "    2) 用 MSVC 的 /fsanitize=address（VS 2019 16.9+）\n"
            "    3) 只做普通构建：-DCMAKE_BUILD_TYPE=Debug（不选 DebugASan 即可）")
    endif()

    message(STATUS "sanitizer 探测通过：-fsanitize=${_san}")
endfunction()

# -----------------------------------------------------------------------------
# 把已经探测过的 sanitizer 选项挂到某个 target 上
# -----------------------------------------------------------------------------
function(w3_apply_sanitizers target)
    if(NOT W3_SANITIZE)
        return()
    endif()

    if(MSVC)
        target_compile_options(${target} PRIVATE /fsanitize=address)
        return()
    endif()

    string(REPLACE ";" "," _san "${W3_SANITIZE}")
    target_compile_options(${target} PRIVATE
        -fsanitize=${_san}
        -fno-omit-frame-pointer
        -g)
    target_link_options(${target} PRIVATE -fsanitize=${_san})
endfunction()
