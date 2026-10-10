#!/bin/sh
# Kindle Tetris installer. Run in KTerm (Wi-Fi on):
#   curl -4fsSL https://raw.githubusercontent.com/awiqu/kindle-app/master/install.sh | sh
# Remove:
#   curl -4fsSL https://raw.githubusercontent.com/awiqu/kindle-app/master/install.sh | sh -s uninstall
set -e

BASE="${TETRIS_BASE:-https://raw.githubusercontent.com/awiqu/kindle-app/master}"
DIR=/mnt/us/extensions/tetris
DOCS=/mnt/us/documents
TMP=/var/tmp/tetris-install.$$

die() { echo "ERROR: $*" >&2; rm -rf "$TMP"; exit 1; }

[ -d /mnt/us ] || die "/mnt/us not found: run this on the Kindle (KTerm)."

if [ "$1" = "uninstall" ]; then
	rm -rf "$DIR" "$DOCS/Tetris.sh"
	echo "Tetris removed."
	exit 0
fi

# TETRIS_INSECURE=1 skips TLS verification (only if the Kindle has no CA certs).
fetch() { # url dest
	echo "  $1"
	if command -v curl >/dev/null 2>&1; then
		if [ -n "$TETRIS_INSECURE" ]; then
			curl -4 -k -fsSL "$1" -o "$2"
		else
			curl -4 -fsSL "$1" -o "$2"
		fi
	elif command -v wget >/dev/null 2>&1; then
		wget -q -O "$2" "$1"
	else
		die "need curl or wget"
	fi
}

sha256_of() { # file
	if command -v sha256sum >/dev/null 2>&1; then
		sha256sum "$1" | awk '{print $1}'
	elif command -v openssl >/dev/null 2>&1; then
		openssl dgst -sha256 "$1" | awk '{print $NF}'
	fi
}

case "$(uname -m)" in
	arm*) ;;
	*) die "not an ARM device ($(uname -m))." ;;
esac

mkdir -p "$TMP" "$DIR" "$DOCS"

echo "Downloading..."
fetch "$BASE/dist/tetris" "$TMP/tetris" || die "download failed (Wi-Fi? URL: $BASE). TLS errors: retry with TETRIS_INSECURE=1"
fetch "$BASE/dist/SHA256SUMS" "$TMP/SHA256SUMS" || die "download failed: SHA256SUMS"
fetch "$BASE/launcher/run.sh" "$TMP/run.sh" || die "download failed: run.sh"
fetch "$BASE/launcher/Tetris.sh" "$TMP/Tetris.sh" || die "download failed: Tetris.sh"

want=$(awk '{print $1}' "$TMP/SHA256SUMS")
got=$(sha256_of "$TMP/tetris")
if [ -z "$got" ]; then
	echo "WARNING: no sha256sum/openssl on this device, checksum NOT verified."
elif [ "$want" != "$got" ]; then
	die "checksum mismatch (want $want, got $got)"
fi
[ "$(dd if="$TMP/tetris" bs=1 skip=1 count=3 2>/dev/null)" = "ELF" ] || die "tetris is not an ELF binary."
head -n 1 "$TMP/run.sh" | grep -q '^#!/bin/sh' || die "run.sh looks wrong."
head -n 1 "$TMP/Tetris.sh" | grep -q '^#!/bin/sh' || die "Tetris.sh looks wrong."

cp "$TMP/tetris" "$DIR/tetris"
cp "$TMP/run.sh" "$DIR/run.sh"
cp "$TMP/Tetris.sh" "$DOCS/Tetris.sh"
chmod 755 "$DIR/tetris" "$DIR/run.sh" "$DOCS/Tetris.sh" 2>/dev/null || true
rm -rf "$TMP"
sync

echo "Installed. Open 'Tetris' in the library (it may take a few seconds to appear)."
echo "First launch asks for 3 taps to calibrate the touch screen."
