# =============================================================================
#  syntax_matrix.ps1  ——  逐一编译各种 lambda 写法，得到"哪些合法 / 哪些非法"的实测矩阵
#
#  用法： powershell -NoProfile -ExecutionPolicy Bypass -File syntax_matrix.ps1
#  输出： day1_syntax_matrix.txt（同时打印到控制台）
#
#  目的：把"mutable 的位置""[=, x] 非法""[*this] 要 C++17"这类说法变成编译器的回答。
# =============================================================================
$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath $PSScriptRoot
$tmp = Join-Path $PWD 'syntax_probe'
New-Item -ItemType Directory -Force -Path $tmp | Out-Null
try { [Console]::OutputEncoding = [System.Text.Encoding]::UTF8 } catch {}

$cases = @(
    @{ n = 'mutable 紧跟参数列表';                   code = 'auto f = []() mutable -> int { return 1; };' }
    @{ n = 'mutable 放在 -> 之后（错序）';            code = 'auto f = []() -> int mutable { return 1; };' }
    @{ n = 'noexcept 与 mutable（mutable 先）';       code = 'auto f = []() mutable noexcept -> int { return 1; };' }
    @{ n = 'noexcept 与 mutable（noexcept 先）';      code = 'auto f = []() noexcept mutable -> int { return 1; };' }
    @{ n = 'constexpr 修饰';                         code = 'auto f = []() constexpr -> int { return 1; };' }
    @{ n = 'mutable + constexpr';                    code = 'auto f = []() mutable constexpr -> int { return 1; };' }
    @{ n = '省略参数列表 ()';                         code = 'int a = 1; auto f = [a] { return a; };' }
    @{ n = '模板 lambda（C++20）';                    code = 'auto f = []<class T>(T x) -> T { return x; };' }
    @{ n = '泛型 lambda（C++14）';                    code = 'auto f = [](auto x) { return x; };' }
    @{ n = 'init capture（C++14）';                   code = 'int y = 1; auto f = [x = y + 1] { return x; };' }
    @{ n = '移动捕获 [p = std::move(q)]';             code = 'std::string q = "a"; auto f = [p = std::move(q)] { return p; };' }
    @{ n = '[=, &b]（默认值捕获 + 显式引用）';         code = 'int a = 1, b = 2; auto f = [=, &b] { return a + b; };' }
    @{ n = '[=, a]（与默认值捕获重复）';               code = 'int a = 1, b = 2; auto f = [=, a] { return a + b; };' }
    @{ n = '[&, &a]（与默认引用捕获重复）';            code = 'int a = 1; auto f = [&, &a] { return a; };' }
    @{ n = '[a, a]（同一变量捕获两次）';               code = 'int a = 1; auto f = [a, a] { return a; };' }
    @{ n = '[&, a]（默认引用 + 显式值捕获）';          code = 'int a = 1; auto f = [&, a] { return a; };' }
    @{ n = '[*this] 拷贝整个对象（C++17）';            code = 'struct S { int v = 1; auto f() { return [*this] { return v; }; } };' }
    @{ n = '[=] 隐式捕获 this（C++20 弃用）';          code = 'struct S { int v = 1; auto f() { return [=] { return v; }; } };' }
    @{ n = '无捕获转函数指针';                         code = 'void (*fp)() = [] {};' }
    @{ n = '有捕获转函数指针（应失败）';               code = 'int a = 1; void (*fp)() = [a] {};' }
    @{ n = 'lambda 直接递归（应失败）';                code = 'auto fib = [](int n) { return n < 2 ? n : fib(n - 1); };' }
    @{ n = 'IIFE 立即调用';                            code = 'const int x = [] { return 42; }();' }
    @{ n = '值捕获后修改，无 mutable（应失败）';        code = 'int a = 1; auto f = [a]() { a = 2; };' }
    @{ n = '值捕获后修改，加 mutable';                  code = 'int a = 1; auto f = [a]() mutable { a = 2; return a; };' }
    @{ n = '引用捕获后修改，无需 mutable';              code = 'int a = 1; auto f = [&a]() { a = 2; };' }
    @{ n = 'constexpr 与 mutable（constexpr 先）';      code = 'auto f = []() constexpr mutable -> int { return 1; };' }
)

$results = New-Object System.Collections.Generic.List[string]
$results.Add("lambda 语法可行性矩阵（g++ 16.1.0 / -std=c++20 / -fsyntax-only）")
$results.Add(("=" * 100))
$i = 0
foreach ($c in $cases) {
    $i++
    $src = Join-Path $tmp "probe_$i.cpp"
    $errFile = Join-Path $tmp "probe_$i.err"
    # 必须放在【块作用域】里：命名空间作用域的 lambda 不能有捕获默认（capture-default），
    # 而且那里的“捕获”会退化成对全局变量的引用 —— 会让本矩阵失真。
    $wrapped = "#include <string>`n#include <utility>`nint main() {`n$($c.code)`nreturn 0; }`n"
    [System.IO.File]::WriteAllText($src, $wrapped, [System.Text.UTF8Encoding]::new($false))

    # 用 cmd 重定向拿原始输出，避免 PowerShell 把 stderr 当异常抛出
    # -Wno-unused-* 只是为了让矩阵不被噪声淹没；保留 -Wall 里的捕获类诊断
    cmd /c "g++ -std=c++20 -Wall -Wno-unused-but-set-variable -Wno-unused-variable -fsyntax-only `"$src`" 2> `"$errFile`""
    $ok = ($LASTEXITCODE -eq 0)
    $err = if (Test-Path $errFile) { [System.IO.File]::ReadAllText($errFile, [System.Text.UTF8Encoding]::new($false)) } else { '' }

    $note = ''
    if (-not $ok) {
        $first = ($err -split "`n" | Where-Object { $_ -match 'error:' } | Select-Object -First 1)
        $note = ($first -replace '.*error:\s*', '').Trim()
    } elseif ($err -match 'warning:') {
        $w = ($err -split "`n" | Where-Object { $_ -match 'warning:' } | Select-Object -First 1)
        $note = '[警告] ' + ($w -replace '.*warning:\s*', '').Trim()
    }
    $status = if ($ok) { 'OK   ' } else { 'ERROR' }
    $results.Add(("{0,2}. {1} {2} {3}" -f $i, $status, $c.n.PadRight(32), $note))
}
$results.Add(("=" * 100))
$results.Add("结论 1（位置）：mutable / constexpr 属于「参数列表之后的修饰符序列」，可互换顺序；")
$results.Add("        noexcept 必须排在 mutable 之后、-> T 之前；返回类型只能在最后。")
$results.Add("结论 2（捕获合法性）：[=, &x] 和 [&, x] 合法；[=, x] / [&, &x] / [x, x] 按标准是 ill-formed，")
$results.Add("        但 GCC 16 只给 -Wall 级警告（换 Clang/MSVC 可能直接报错）→ 实际编码一律避免。")
$results.Add("结论 3（值捕获是 const）：无 mutable 时值捕获不可修改（ERROR）；加 mutable 或改引用捕获才行。")
$results.Add("结论 4（函数指针）：无捕获才有到函数指针的隐式转换；有捕获直接 ERROR。")
$results.Add("结论 5（递归）：lambda 不能直接递归（闭包类型在 auto 推导完成前不可用）——用 std::function 或 Y 组合子。")

$out = Join-Path $PWD 'day1_syntax_matrix.txt'
[System.IO.File]::WriteAllLines($out, $results, [System.Text.UTF8Encoding]::new($false))
$results | ForEach-Object { Write-Host $_ }
Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
