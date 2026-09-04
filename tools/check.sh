#!/usr/bin/env sh
set -eu

preset="${1:-host-gcc}"

require_command()
{
    command -v "$1" >/dev/null 2>&1 || {
        echo "$1 is required" >&2
        exit 1
    }
}

case "$preset" in
    host-gcc)
        compiler="gcc-13"
        ;;
    host-clang | host-sanitize)
        compiler="clang-21"
        ;;
    *)
        echo "unknown preset: $preset" >&2
        exit 1
        ;;
esac

for command_name in cmake ninja git rg clang-format-21 clang-tidy-21 run-clang-tidy-21 \
    cppcheck "$compiler"; do
    require_command "$command_name"
done

cmake --version | sed -n '1p'
ninja --version
"$compiler" --version | sed -n '1p'
clang-format-21 --version
clang-tidy-21 --version | sed -n '1p'
cppcheck --version

empty_tree="4b825dc642cb6eb9a060e54bf8d69288fbee4904"
git diff --check "$empty_tree" HEAD
git diff --cached --check
git diff --check

if ! git grep -Ilz -e '' | xargs -0 -r sh -c '
    status=0
    for file do
        if [ -s "$file" ] && [ "$(tail -c 1 "$file" | wc -l)" -ne 1 ]; then
            echo "$file: missing final newline" >&2
            status=1
        fi
    done
    exit "$status"
' sh; then
    exit 1
fi

rg --files -0 -g '*.c' -g '*.h' -g '!build/**' |
    xargs -0 -r clang-format-21 --style=file --dry-run --Werror

cmake --preset "$preset"
cmake --build --preset "$preset"
ctest --preset "$preset"

build_dir="build/$preset"
run-clang-tidy-21 -p="$build_dir" -clang-tidy-binary clang-tidy-21 -quiet

cppcheck --enable=warning,style,performance,portability --error-exitcode=1 \
    --inline-suppr --language=c --std=c11 \
    --suppress=missingIncludeSystem -i build .
