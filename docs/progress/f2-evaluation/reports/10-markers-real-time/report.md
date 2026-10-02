# 10-marker benchmark (real time)

- Tracker: synchronized multi-marker tracker
- Pacing: real time at 30.0 fps (frames may be skipped, as in the app)
- Scored frames: 300 after 30 warm-up frames
- Found rate (mean over markers): 100.0 %
- Outline error vs. the frame on screen: RMSE 11.811 px, mean 8.731, median 6.167, p95 27.148, max 79.308 (n=3000)
- Outline error vs. the frame the tracker processed: RMSE 10.448 px, mean 6.768, median 3.988, p95 26.670, max 79.790 (n=3000)
- Lag, frames between processed and shown: RMSE 4.654 frames, mean 3.603, median 2.500, p95 9.000, max 12.000 (n=3000)

| # | Marker | Found | Mean outline error (px) |
|---|---|---|---|
| 1 | yejin.jpg | 100.0 % | 5.73 |
| 2 | fish.jpg | 100.0 % | 6.48 |
| 3 | cola.jpg | 100.0 % | 15.85 |
| 4 | grafi.jpg | 100.0 % | 6.25 |
| 5 | suji.jpg | 100.0 % | 9.59 |
| 6 | hyojoo.jpg | 100.0 % | 12.55 |
| 7 | iu1.jpg | 100.0 % | 6.00 |
| 8 | mina1.jpg | 100.0 % | 6.00 |
| 9 | minjung1.jpg | 100.0 % | 6.59 |
| 10 | minjung4.jpg | 100.0 % | 12.26 |
