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

  ![video_0_1s](https://github.com/user-attachments/assets/a94411e0-9e91-4882-be64-cbf49b7f1442)


- **Outputs**  
  1. The resampled frame is automatically stored as PNG with a timestamped name (possible as EPS if ImageMagick is installed in your system).
  2. A textfile with the azimuth pick tip counts, x-coordinates and their heights above baseline. Estimmated cutting line distance in also provided.

  ![Capture](https://github.com/user-attachments/assets/4a3a4593-2df6-4f46-8a95-f98fd86955d5)

## 🧪 How to Run
  1. Download and extract the application folder
  2. Open the file: /application/Rock Cutter Wear Estimation.exe
  3. Choose the video number (0-2) you want and press enter (default is video_0.mp4)
  4. Tune the brightness and binary threshold to your preference ensuring no noise contours forming in the video (suggested tuning is written on the console in video selection menu).
  5. After tuning wait for the whole video to run at least once to record contours from every frame.
  6. Click on the video 2-3 times to ensure the azimuth contour line on the resampled frame is consistent.
  7. The measurement file and PNG file is exported in “Outputs” folder (folder will be automatically created if it doesn't exist in the application folder).

## 🛠️ Requirements for Code base (Debug)

- OpenCV (v4.x)
- C++14 compatible compiler
- ImageMagick (optional, for EPS export if used)

# License
This project is open-source and available under the MIT License.

# Author notes
If you liked this project, please leave a star! Feel free to reach out to me on [LinkedIn](https://www.linkedin.com/in/nibras-sajjad/) if you would like to connect.
