#!/bin/sh
# Kindle Tetris launcher: stops the Kindle UI, runs the game, brings the UI back.
# Usage: run.sh [--calibrate]
DIR=/mnt/us/extensions/tetris
LOG=$DIR/tetris.log
PIDFILE=/var/tmp/tetris.pid
BIN=$DIR/tetris

if [ "$1" != "--detached" ]; then
	if [ ! -x "$BIN" ]; then
		echo "Tetris is not installed ($BIN missing)."
		exit 1
	fi
	# Stopping the UI also stops whatever started us (the scriptlet runner),
	# so continue in a fresh session that outlives it.
	[ -f "$LOG" ] && [ "$(wc -c <"$LOG")" -gt 100000 ] && : >"$LOG"
	if command -v setsid >/dev/null 2>&1; then
		setsid sh "$0" --detached "$@" </dev/null >>"$LOG" 2>&1 &
	else
		nohup sh "$0" --detached "$@" </dev/null >>"$LOG" 2>&1 &
	fi
	exit 0
fi
shift

if [ -f "$PIDFILE" ] && kill -0 "$(cat "$PIDFILE")" 2>/dev/null; then
	echo "already running"
	exit 0
fi
echo $$ >"$PIDFILE"

UPSTART=0
if command -v stop >/dev/null 2>&1 && [ -d /etc/upstart ]; then
	UPSTART=1
fi

restored=0
restore() {
	[ "$restored" = 1 ] && return
	restored=1
	lipc-set-prop com.lab126.powerd preventScreenSaver 0 2>/dev/null
	killall -CONT awesome 2>/dev/null
	cd / || true
	if [ "$UPSTART" = 1 ]; then
		start lab126_gui
	else
		/etc/init.d/framework start
	fi
	rm -f "$PIDFILE"
}
trap restore EXIT
trap 'exit 1' INT TERM HUP

echo "=== $(date) starting tetris $* ==="
lipc-set-prop com.lab126.powerd preventScreenSaver 1 2>/dev/null
if [ "$UPSTART" = 1 ]; then
	stop lab126_gui
else
	/etc/init.d/framework stop
fi
killall -STOP awesome 2>/dev/null
sleep 1

"$BIN" "$@"
echo "=== tetris exited with $? ==="
