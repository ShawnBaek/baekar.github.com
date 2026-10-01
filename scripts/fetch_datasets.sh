#!/usr/bin/env bash
# Downloads the public datasets used by F2 and summarises them with
# baekar_eval.
#
#   scripts/fetch_datasets.sh <build-dir> [data-dir]
#
# data-dir defaults to ./data (git-ignored). Already downloaded datasets are
# skipped. Reports go to docs/progress/f2-evaluation/reports/<dataset>/.
set -euo pipefail

BUILD=$(cd "$1" && pwd)
ROOT=$(cd "$(dirname "$0")/.." && pwd)
DATA=${2:-$ROOT/data}
REPORTS="$ROOT/docs/progress/f2-evaluation/reports"
mkdir -p "$DATA"

# name|url|archive type
DATASETS=(
  "rgbd_dataset_freiburg1_xyz|https://cvg.cit.tum.de/rgbd/dataset/freiburg1/rgbd_dataset_freiburg1_xyz.tgz|tgz"
  "rgbd_dataset_freiburg2_desk|https://cvg.cit.tum.de/rgbd/dataset/freiburg2/rgbd_dataset_freiburg2_desk.tgz|tgz"
  "MH_01_easy|http://robotics.ethz.ch/~asl-datasets/ijrr_euroc_mav_dataset/machine_hall/MH_01_easy/MH_01_easy.zip|zip"
)

for entry in "${DATASETS[@]}"; do
  IFS='|' read -r name url kind <<<"$entry"
  target="$DATA/$name"
  if [ ! -d "$target" ]; then
    echo "Downloading $name"
    archive="$DATA/$name.$kind"
    curl -fL --retry 4 --retry-delay 2 -o "$archive" "$url"
    if [ "$kind" = tgz ]; then
      tar -xzf "$archive" -C "$DATA"
    else
      mkdir -p "$target" && unzip -q "$archive" -d "$target"
    fi
    rm -f "$archive"
  fi
  "$BUILD/baekar_eval" dataset "$target" --out "$REPORTS/$name"
done
