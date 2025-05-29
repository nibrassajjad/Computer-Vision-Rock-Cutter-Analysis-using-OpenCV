# Computer Vision Project: Rock Cutter Wear Analysis using OpenCV/C++ 

This OpenCV (coded in C++) application provides an automated video analysis solution for rotating rock cutter drums, enabling real-time detection of pick tips, azimuth contours, and cutting height estimation from raw industrial footage.

![animation](https://github.com/user-attachments/assets/d2343e3e-5cf7-4714-b3f8-1afc8c7af6ce)

## 🚀 Features

- **Real-time Contour Detection**  
  Automatically identifies the contour lines of the rotating drum in the video for user-verification.

- **Dynamic Brightness & Threshold Control**  
  Enables live tuning for frame-by-frame adjustments without code changes.

- **Further analysis details outputted on a 4-times resampled zoomed frame**  
  Provided visual statistics of a clicked frame.
  
- **Features detected on resampled frame**  
  1. Baseline of the drum
  2. Azimuth contour line based on all visible picks of the cutter (azimuth is a case when a pick achieves its highest possible point for its corresponding x-coordinate value)
  3. Estimated pick tip locations of the cutter and their corresponding height from baseline
  4. Precise location of a pick in azimuth on clicked frame along with its corresponding height from baseline
  5. Visual triangles to point out picks that are in azimuth
  6. An estimated equidistant cutting line with their corresponding x-coordinates

- **Outputs**  
  1. The resampled frame is automatically stored as PNG with a timestamped name(possible as EPS if ImageMagick is installed in your system).
  2. A textfile with the azimuth pick tip counts, x-coordinates and their heights above baseline. Estimmated cutting line distance in also provided.
