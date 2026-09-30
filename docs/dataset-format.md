# BaekAR dataset format (version 1)

A BaekAR dataset is a folder that the engine replays with `--replay DIR` and that `baekar_eval` scores. `--record DIR` writes it. The iPhone capture app (roadmap F3) writes the same layout. Readers are in `src/adapters/frame_source/DatasetSources.cpp`.

```text
DIR/
  baekar_dataset.json   metadata (required)
  frames.csv            one row per image (required)
  rgb/000000.png        colour images, 8-bit BGR/RGB PNG or JPEG
  depth/000000.png      optional depth, 16-bit PNG, millimetres, 0 = unknown
  imu.csv               optional inertial samples
```

## baekar_dataset.json

```json
{
  "format": "baekar-dataset",
  "version": 1,
  "device": "iPhone 17 Pro, Main camera",
  "intrinsics": { "fx": 1450.2, "fy": 1450.2, "cx": 959.5, "cy": 719.5,
                  "width": 1920, "height": 1440,
                  "distortion": [0, 0, 0, 0, 0] },
  "depth_scale": 1000.0,
  "pose_source": "arkit"
}
```

- `intrinsics` describe the stored `rgb/` images. `distortion` is OpenCV's `k1 k2 p1 p2 k3`. It is optional; the default is no distortion.
- `depth_scale` is the stored depth units per metre (1000 means millimetres).
- `pose_source` names where `frames.csv` poses come from: `arkit`, `groundtruth`, `none`, …

## frames.csv

```text
timestamp,rgb,depth,tx,ty,tz,qx,qy,qz,qw
12.345678901,rgb/000000.png,depth/000000.png,0.01,0.02,0.03,0,0,0,1
```

- `timestamp` is seconds on the device clock (monotonic within the dataset).
- `depth` may be empty.
- The pose is `world_from_camera` in the OpenCV camera convention (x right, y down, z forward), metres, Hamilton quaternion. Leave all seven fields empty when a frame has no pose. ARKit uses a different camera convention (x right, y up, z backward); the capture app converts before writing.

## imu.csv

```text
timestamp,gx,gy,gz,ax,ay,az
12.340000000,0.001,-0.002,0.000,0.02,-9.80,0.10
```

Gyroscope in rad/s and accelerometer in m/s², in the device frame, on the same clock as `frames.csv`. The reader attaches the samples that fall after the previous frame (and at or before the current one) to each frame.

## Other formats `--replay` understands

| Detected by | Format |
|---|---|
| `baekar_dataset.json` | this format |
| `rgb.txt` + `depth.txt` | TUM RGB-D (colour, depth ÷ 5000, `groundtruth.txt`; intrinsics from the `freiburg1/2/3` folder name) |
| `mav0/cam0/data.csv` | EuRoC MAV (cam0 greyscale, `imu0`, ground truth converted to the camera with `T_BS`) |
| anything else | a folder of `*.png` / `*.jpg` images in name order |
