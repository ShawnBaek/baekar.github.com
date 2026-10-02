# 10-marker benchmark (every frame)

- Tracker: synchronized multi-marker tracker
- Pacing: every frame (the tracker finishes each frame before the next)
- Scored frames: 300 after 30 warm-up frames
- Found rate (mean over markers): 99.8 %
- Outline error vs. the frame on screen: RMSE 7.287 px, mean 5.299, median 3.982, p95 13.174, max 49.148 (n=2994)
- Outline error vs. the frame the tracker processed: RMSE 7.287 px, mean 5.299, median 3.982, p95 13.174, max 49.148 (n=2994)
- Latency, submit to published: RMSE 70.377 ms, mean 33.434, median 6.082, p95 172.917, max 278.870 (n=330)

| # | Marker | Found | Mean outline error (px) |
|---|---|---|---|
| 1 | yejin.jpg | 100.0 % | 3.62 |
| 2 | fish.jpg | 98.0 % | 10.28 |
| 3 | cola.jpg | 100.0 % | 3.51 |
| 4 | grafi.jpg | 100.0 % | 4.23 |
| 5 | suji.jpg | 100.0 % | 8.08 |
| 6 | hyojoo.jpg | 100.0 % | 5.73 |
| 7 | iu1.jpg | 100.0 % | 4.04 |
| 8 | mina1.jpg | 100.0 % | 3.23 |
| 9 | minjung1.jpg | 100.0 % | 5.01 |
| 10 | minjung4.jpg | 100.0 % | 5.37 |
