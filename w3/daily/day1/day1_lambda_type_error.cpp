// =============================================================================
//  day1_lambda_type_error.cpp   ——  【故意编译不过】用于抓"返回类型推导失败"的真实报错
//
//  复现： g++ -std=c++20 -fsyntax-only day1_lambda_type_error.cpp
//  报错存档： day1_lambda_type_error.txt
//
//  周一知识点：lambda 的返回类型是"推导"出来的；当多个 return 语句推导出不同类型时，
//  编译期直接失败，必须显式写 -> T。
// =============================================================================
#include <iostream>

int main() {
    // (1) 多个 return 推导出不同类型（int vs double）→ 推导失败
    auto bad = [](int x) {
        if (x > 0) return 1;     // int
        return 2.0;              // double  ← 冲突
    };
    std::cout << bad(1) << '\n';

    // (2) 正确写法：显式指定返回类型
    auto good = [](int x) -> double {
        if (x > 0) return 1;
        return 2.0;
    };
    std::cout << good(1) << '\n';
    return 0;
}
