# 1-marker benchmark (real time)

- Tracker: legacy single-marker tracker (2012)
- Pacing: real time at 30.0 fps (frames may be skipped, as in the app)
- Scored frames: 300 after 30 warm-up frames
- Found rate (mean over markers): 100.0 %
- Outline error vs. the frame on screen: RMSE 3.307 px, mean 2.906, median 2.091, p95 6.109, max 9.330 (n=300)
- Outline error vs. the frame the tracker processed: RMSE 0.990 px, mean 0.920, median 0.838, p95 1.538, max 3.551 (n=300)
- Lag, frames between processed and shown: RMSE 1.581 frames, mean 1.487, median 1.000, p95 2.000, max 3.000 (n=300)

| # | Marker | Found | Mean outline error (px) |
|---|---|---|---|
| 1 | yejin.jpg | 100.0 % | 2.91 |
