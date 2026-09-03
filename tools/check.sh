#!/usr/bin/env sh
set -eu

preset="${1:-host-gcc}"

command -v cmake >/dev/null 2>&1 || { echo "cmake is required" >&2; exit 1; }
command -v ninja >/dev/null 2>&1 || { echo "ninja is required" >&2; exit 1; }
command -v clang-format-21 >/dev/null 2>&1 || {
    echo "clang-format-21 is required" >&2
    exit 1
}
if command -v clang-tidy-21 >/dev/null 2>&1; then
    clang_tidy="clang-tidy-21"
elif command -v clang-tidy >/dev/null 2>&1; then
    clang_tidy="clang-tidy"
else
    echo "clang-tidy is required" >&2
    exit 1
fi
command -v cppcheck >/dev/null 2>&1 || { echo "cppcheck is required" >&2; exit 1; }

git diff --check

c_sources="$(rg --files -g '*.c' -g '*.h' -g '!build/**' || true)"
if [ -n "$c_sources" ]; then
    printf '%s\n' "$c_sources" | xargs clang-format-21 --style=file --dry-run --Werror
fi

cmake --preset "$preset"
cmake --build --preset "$preset"
ctest --preset "$preset"

build_dir="build/$preset"
if [ -n "$c_sources" ]; then
    printf '%s\n' "$c_sources" | while IFS= read -r source; do
        "$clang_tidy" -p="$build_dir" "$source"
    done
fi

cppcheck --enable=warning,style,performance,portability --error-exitcode=1 \
    --inline-suppr --language=c --std=c11 \
    --suppress=missingIncludeSystem -i build .
