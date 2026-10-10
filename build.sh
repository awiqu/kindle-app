#!/bin/sh
# Cross-compiles dist/tetris for Kindle (ARMv7 soft-float, static musl) using zig.
# Needs: python3, git, make. Zig comes from the `ziglang` pip wheel.
# Usage: ./build.sh            (output: dist/tetris, dist/SHA256SUMS)
set -eu

ROOT=$(cd "$(dirname "$0")" && pwd)
WORK="${BUILD_DIR:-$ROOT/.build}"
FBINK_REV=886f25f13368859ad8a899b88d04c26e19cda32e

mkdir -p "$WORK/bin" "$ROOT/dist"

if [ ! -x "$WORK/zigenv/bin/python" ]; then
	python3 -m venv "$WORK/zigenv"
	"$WORK/zigenv/bin/pip" install -q ziglang
fi
ZIG="$WORK/zigenv/bin/python -m ziglang"

wrap() { # name, command
	printf '#!/bin/sh\nexec %s "$@"\n' "$2" >"$WORK/bin/$1"
	chmod +x "$WORK/bin/$1"
}
wrap zcc "$ZIG cc -target arm-linux-musleabi -mcpu=cortex_a7 -mfloat-abi=soft"
wrap zar "$ZIG ar"
wrap zranlib "$ZIG ranlib"

if [ ! -d "$WORK/fbink/.git" ]; then
	GIT_LFS_SKIP_SMUDGE=1 git clone https://github.com/NiLuJe/FBInk "$WORK/fbink"
fi
(
	cd "$WORK/fbink"
	git checkout -q "$FBINK_REV"
	git submodule update --init --recursive -q
	make clean >/dev/null 2>&1 || true
	make KINDLE=1 MINIMAL=1 DRAW=1 BITMAP=1 INPUT=1 \
		CC="$WORK/bin/zcc" AR="$WORK/bin/zar" RANLIB="$WORK/bin/zranlib" STRIP=true staticlib
)

"$WORK/bin/zcc" -std=gnu11 -Os -s -Wall -Wextra -static \
	-I"$WORK/fbink" -o "$ROOT/dist/tetris" "$ROOT/src/tetris.c" "$WORK/fbink/Release/libfbink.a"

(cd "$ROOT/dist" && sha256sum tetris >SHA256SUMS)
ls -l "$ROOT/dist"
