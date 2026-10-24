#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

# Usage: ./flash.sh [/dev/cu.usbmodemXXXX] [--yes]
IMAGE="build/FoloToy-AI-Passport-full.bin"
PORT="${1:-}"
CONFIRM="${2:-}"

if [[ ! -f "$IMAGE" ]]; then
  echo "缺少 $IMAGE；先在已激活的 ESP-IDF 环境里运行 ./tools/validate.sh --firmware。" >&2
  exit 1
fi

if [[ -z "$PORT" ]]; then
  PORT="$(python3 - <<'PY'
import serial.tools.list_ports
ports = [p.device for p in serial.tools.list_ports.comports()
         if 'usbmodem' in p.device or 'usbserial' in p.device or 'wchusbserial' in p.device]
print(ports[0] if len(ports) == 1 else '')
PY
)"
fi

if [[ -z "$PORT" ]]; then
  echo "没有找到唯一的 USB 串口。先连好设备，或用：$0 /dev/cu.usbmodemXXXX --yes" >&2
  exit 1
fi

if command -v shasum >/dev/null 2>&1; then
  HASH="$(shasum -a 256 "$IMAGE" | awk '{print $1}')"
else
  HASH="$(sha256sum "$IMAGE" | awk '{print $1}')"
fi
SIZE="$(stat -f%z "$IMAGE" 2>/dev/null || stat -c%s "$IMAGE")"
echo "镜像   $IMAGE"
echo "SHA256 $HASH"
echo "大小   $SIZE bytes"
echo "端口   $PORT"
echo ""
echo "警告：从 0x0 刷入合并镜像可能重置 NVS 中的已存数据（Wi-Fi/服务器配置）。"
echo "本应用首次启动本来就需要配网；如需保留旧数据，改用分段刷写。"
echo ""

if [[ "$CONFIRM" != "--yes" ]]; then
  echo "确认刷写请重跑：$0 $PORT --yes" >&2
  exit 1
fi

if ! command -v idf.py >/dev/null 2>&1; then
  echo "请先 source ESP-IDF 5.5.3 的 export.sh。" >&2
  exit 1
fi

python -m esptool --chip esp32c3 -p "$PORT" -b 460800 write_flash 0x0 "$IMAGE"
echo "刷写完成。打开串口监视：idf.py -p $PORT monitor"
