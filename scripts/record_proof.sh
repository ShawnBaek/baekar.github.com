#!/usr/bin/env bash
# Records a proof run for a roadmap milestone on a headless X server.
#
#   scripts/record_proof.sh <build-dir> <milestone> [extra BaekAR args per scenario...]
#
# Writes docs/progress/<milestone>/:
#   proof.mp4    every scenario, with a caption bar (H.264)
#   proof.gif    a short, small preview for the README
#   results.txt  CTest summary and per-scenario run summary
#
# Needs: Xvfb, ffmpeg (with drawtext), xdotool. Linux only.
set -euo pipefail

BUILD=$(cd "$1" && pwd)
MILESTONE=$2
shift 2
ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT="$ROOT/docs/progress/$MILESTONE"
WORK=$(mktemp -d)
FONT=/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf
DISPLAY_NUM=:97
mkdir -p "$OUT"

# Standard scenarios; extra ones can be passed as quoted "Title|args" strings.
SCENARIOS=(
  "1 marker - legacy tracker, CCamera pose, HandyAR|--simulate-marker yejin.jpg"
  "10 markers - synchronized multi-marker tracker|--simulate-marker yejin.jpg --simulate-marker fish.jpg --simulate-marker cola.jpg --simulate-marker grafi.jpg --simulate-marker suji.jpg --simulate-marker hyojoo.jpg --simulate-marker iu1.jpg --simulate-marker mina1.jpg --simulate-marker minjung1.jpg --simulate-marker minjung4.jpg"
)
for extra in "$@"; do SCENARIOS+=("$extra"); done

Xvfb "$DISPLAY_NUM" -screen 0 640x480x24 >/dev/null 2>&1 &
XVFB=$!
trap 'kill $XVFB 2>/dev/null; rm -rf "$WORK"' EXIT
sleep 1

{
  echo "BaekAR proof: $MILESTONE ($(git -C "$ROOT" rev-parse --short HEAD), $(date -u +%Y-%m-%d))"
  echo
  echo "\$ ctest"
  (cd "$BUILD" && ctest 2>&1 | grep -E 'Test +#|tests passed|tests failed')
  echo
  echo "Scenarios (ESC to quit; exit code 0 means a clean shutdown):"
} > "$OUT/results.txt"

index=0
for scenario in "${SCENARIOS[@]}"; do
  title=${scenario%%|*}
  args=${scenario#*|}
  index=$((index + 1))
  log="$WORK/$index.log"
  # shellcheck disable=SC2086
  (cd "$BUILD" && DISPLAY=$DISPLAY_NUM ./BaekAR $args --frames 100000 >"$log" 2>&1) &
  app=$!
  sleep 3
  ffmpeg -loglevel error -y -f x11grab -draw_mouse 0 -framerate 30 -video_size 640x480 \
    -i "$DISPLAY_NUM.0" -t 8 \
    -vf "drawbox=x=0:y=0:w=640:h=34:color=black@0.65:t=fill,drawtext=fontfile=$FONT:text='$MILESTONE - $title':fontcolor=white:fontsize=15:x=10:y=9" \
    -c:v libx264 -pix_fmt yuv420p -preset slow -crf 28 "$WORK/$index.mp4"
  DISPLAY=$DISPLAY_NUM xdotool key --window "$(DISPLAY=$DISPLAY_NUM xdotool search --name BaekAR | head -1)" Escape || kill "$app"
  code=0; wait "$app" || code=$?
  printf '  %-50s exit=%d  %s\n' "$title" "$code" "$(grep -E 'rendered' "$log" | sed 's/BaekAR: //')" >> "$OUT/results.txt"
done

(cd "$WORK" && ls [0-9]*.mp4 | sort -n | sed "s/^/file '/; s/$/'/" > list.txt &&
  ffmpeg -loglevel error -y -f concat -safe 0 -i list.txt -c copy "$OUT/proof.mp4")
ffmpeg -loglevel error -y -i "$OUT/proof.mp4" \
  -vf "fps=6,scale=360:-1:flags=lanczos,split[a][b];[a]palettegen=max_colors=64[p];[b][p]paletteuse=dither=bayer" \
  "$OUT/proof.gif"
ls -la "$OUT"
cat "$OUT/results.txt"
