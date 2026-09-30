# 1-marker benchmark (every frame)

- Tracker: legacy single-marker tracker (2012)
- Pacing: every frame (the tracker finishes each frame before the next)
- Scored frames: 300 after 30 warm-up frames
- Found rate (mean over markers): 100.0 %
- Outline error vs. the frame on screen: RMSE 0.989 px, mean 0.919, median 0.841, p95 1.538, max 3.551 (n=300)
- Outline error vs. the frame the tracker processed: RMSE 0.989 px, mean 0.919, median 0.841, p95 1.538, max 3.551 (n=300)
- Latency, submit to published: RMSE 31.233 ms, mean 30.728, median 29.273, p95 41.505, max 51.326 (n=330)

| # | Marker | Found | Mean outline error (px) |
|---|---|---|---|
| 1 | yejin.jpg | 100.0 % | 0.92 |
