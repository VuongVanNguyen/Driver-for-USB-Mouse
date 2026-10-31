#!/usr/bin/env bash
# Chạy trên WSL: đồng bộ code sang Pi rồi build thử (chưa cần commit).
# Dùng: PI_HOST=<user>@<ip-của-pi> scripts/sync_to_pi.sh
#   WSL2 thường không phân giải được tên *.local, nên dùng IP (chạy "hostname -I" trên Pi).
set -euo pipefail

if [[ -z ${PI_HOST:-} ]]; then
	echo "Chưa đặt PI_HOST, vd: PI_HOST=pi@192.168.1.10 $0" >&2
	exit 1
fi
PI_DIR="${PI_DIR:-usb_mouse_driver}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

rsync -av --delete --exclude .git --exclude-from="$ROOT/.gitignore" \
	"$ROOT/" "$PI_HOST:$PI_DIR/"
ssh "$PI_HOST" "cd $PI_DIR && make"
