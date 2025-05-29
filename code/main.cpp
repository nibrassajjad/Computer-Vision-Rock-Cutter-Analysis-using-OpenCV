#include <opencv2/opencv.hpp>  // Include OpenCV's core functionality
#include <iostream>            // Include standard I/O stream library
#include <opencv2/highgui.hpp> // Include OpenCV's high-level GUI functions
#include <algorithm>           // for sort, abs
#include <numeric>             // for accumulate
#include <fstream>             // For file output
//#include <Magick++.h>          // Include ImageMagick++ for EPS conversion

using namespace cv;            // Use the cv namespace to simplify OpenCV code
using namespace std;           // Use the std namespace for standard library

// Global variables to store the current and selected video frames
Mat currentFrame;              // Frame currently being displayed in the video loop
Mat selectedFrame;             // Frame selected by clicking with the mouse
double currentTimestamp = 0.0; // Store current video timestamp in ms
float fixedBaselineY = -1;      // Will store the uppermost baseline detected
string baseName = "frame";      // fallback name, will be set in main()

// Global variables for trackbar values
int brightnessValue = 11;        // Brightness offset, default = 11
int thresholdValue = 101;        // Binary threshold, default = 101
int prevBrightnessValue = -1;   // Previous brightness value before user adjustment
int prevThresholdValue = -1;    // Previous threshold value before user adjustment

// Global for accumulated binary
Mat accumulatedBinary;           // Stores merged binary masks across frames
bool isFirstBinary = true;       // Flag to initialize accumulatedBinary

vector<Point> recordedRedCircles; // Stores red dot positions across clicks

// Utility function to draw dashed lines
void drawDashedLine(Mat& img, Point start, Point end, Scalar color, int dashLength = 10, int gapLength = 5, int thickness = 1) {
    for (int y = start.y; y < end.y; y += dashLength + gapLength) {
        int yEnd = min(y + dashLength, end.y);
        line(img, Point(start.x, y), Point(end.x, yEnd), color, thickness);
    }
}

// Mouse callback function: triggered when user clicks on the video frame
void onMouse(int event, int x, int y, int flags, void* userdata) {
    // Only respond to left-click and if a frame exists
    if (event == EVENT_LBUTTONDOWN && !currentFrame.empty()) {

        // Reset red dot memory for every click
        recordedRedCircles.clear();

        // Clone the current frame into selectedFrame
        selectedFrame = currentFrame.clone();

        // Create a 4-fold xy-resampled(4x zoomed) version of the selected frame
        Mat zoomed;
        resize(selectedFrame, zoomed, Size(), 4.0, 4.0, INTER_LINEAR);  // INTER_LINEAR = smooth scaling

        // Convert current timestamp from milliseconds to seconds
        int timestamp_sec = static_cast<int>(currentTimestamp / 1000.0);

        string outTextPath = "Outputs/" + baseName + "_measurements.txt"; // Measurement file output
        ofstream outFile(outTextPath); // Overwrite data with every user click

        // Compose a filename using video name and timestamp
        // string filename = baseName + "_" + to_string(timestamp_sec) + "s.png";
        string filename = "Outputs/" + baseName + "_" + to_string(timestamp_sec) + "s.png";


        // ------- AZIMUTH CONTOUR BUILD START ------- //

       // Create the scaled original image first
        Mat originalZoomed;
        resize(selectedFrame, originalZoomed, Size(), 4.0, 4.0, INTER_LINEAR);

        vector<vector<Point>> mergedContours;  // Single declaration

        if (!accumulatedBinary.empty()) {
            findContours(accumulatedBinary, mergedContours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

            for (const auto& contour : mergedContours) {
                // --- Original: Draw detailed blue contour (after green-to-blue pass) ---
                vector<Point> scaledContour;
                for (const Point& pt : contour) {
                    scaledContour.push_back(pt * 4);
                }
                drawContours(zoomed, vector<vector<Point>>{scaledContour}, -1, Scalar(0, 255, 0), 2);


                // ------- AZIMUTH PICK TIP DETECTION START ------- //

                // --- Draw violet simplified contour using approxPolyDP ---
                vector<Point> approx;
                double epsilon = 0.0025 * arcLength(contour, true);  // Tuning parameter: smaller epsilon = more detail
                approxPolyDP(contour, approx, epsilon, true);

                // Scale approximated poly corner point           
                vector<Point> redCirclePoints;
                for (const auto& pt : approx) {
                    Point scaledPt = pt * 4;
                    redCirclePoints.push_back(scaledPt);  // Store the red dot position                  
                }

                // (Optional) Draw the simplified contour in violet (BGR: 255, 0, 255)
                //drawContours(zoomed, vector<vector<Point>>{scaledApprox}, -1, Scalar(255, 0, 255), 2);


                // Filter close red dots to retain only the highest one (smallest y) in each horizontal neighborhood
                // Red dots within 80px of each other are considered overlapping candidates
                vector<bool> replaced(redCirclePoints.size(), false);

                // Loop through each red circle point
                for (size_t i = 0; i < redCirclePoints.size(); ++i) {
                    if (replaced[i]) continue; // Skip if already replaced

                    Point pt1 = redCirclePoints[i];  // Current red circle point

                    // Compare pt1 with the rest of the points to its right
                    for (size_t j = i + 1; j < redCirclePoints.size(); ++j) {
                        if (replaced[j]) continue;

                        Point pt2 = redCirclePoints[j];

                        // Check if the two points are horizontally close within 80 px
                        if (abs(pt2.x - pt1.x) <= 80) {
                            // If both points are horizontally close, keep the one higher on frame
                            if (pt1.y < pt2.y) {
                                replaced[j] = true; // if pt1 is higher (smaller y), keep pt1, discard pt2
                            }
                            else {
                                replaced[i] = true; // if pt2 is higher, discard pt1
                                break;  // Exit early since pt1 is no longer valid
                            }
                        }
                    }
                }
                // ------- AZIMUTH PICK TIP DETECTION END ------- //


                // ------- AZIMUTH PICK TIP COORDINATE RECORD START ------- //

                // After filtering: record coordinates only final red dots
                for (size_t i = 0; i < redCirclePoints.size(); ++i) {
                    if (!replaced[i]) {
                        recordedRedCircles.push_back(redCirclePoints[i]);
                    }
                }


                // Optional: Print final red circle coordinates after filtering
                //cout << "Final red circle coordinates after filtering:" << endl;
                //for (const auto& pt : recordedRedCircles) {
                //    cout << "(" << pt.x << ", " << zoomed.rows - pt.y << ")" << endl;
                //}

                if (fixedBaselineY != -1) {
                    int zoomedBaselineY = fixedBaselineY * 4;
                    cout << "\n[AZIMUTH] Pick Count, X-position and Height in Azimuth Contour:\n\n";
                    cout << "  Azimuth Pick Count in Video: " << recordedRedCircles.size() << endl;

                    outFile << "[AZIMUTH] Pick Count, X-position and Height in Azimuth Contour:\n\n";
                    outFile << "  Azimuth Pick Count in Video: " << recordedRedCircles.size() << endl;

                    for (const auto& pt : recordedRedCircles) {
                        int height = zoomedBaselineY - pt.y;
                        cout << "  Azimuth Pick at x-position " << pt.x << " px with Pick Height: " << height << " px\n";
                        outFile << "  Azimuth Pick at x-position " << pt.x << " px with Pick Height: " << height << " px\n";

                        // Draw red circle
                        circle(zoomed, pt, 4, Scalar(0, 0, 255), FILLED);  // Red dot

                        // Coordinate label
                        string coordText = "(" + to_string(pt.x) + "," + to_string(zoomed.rows - pt.y) + ")";
                        int textX = pt.x + 5;
                        int textY = pt.y - 5;

                        if (pt.x > zoomed.cols - 100) {
                            textX = pt.x - 70;   // move left if near right edge
                            textY = pt.y + 15;
                        }

                        putText(zoomed, coordText, Point(textX, textY), FONT_HERSHEY_PLAIN, 0.9, Scalar(255, 255, 255), 1);

                        // Height label (placed slightly below the coordinate label)
                        string heightText = "H: " + to_string(height) + "px";
                        putText(zoomed, heightText, Point(textX, textY + 12), FONT_HERSHEY_PLAIN, 0.9, Scalar(180, 180, 255), 1);
                    }
                }


                // ------- AZIMUTH PICK TIP COORDINATE RECORD ENDS ------- //


                // ------- PRELIMINARY CONTOUR + PICK AZIMUTH MATCHING START ------- //

                // Step 1: Get pick tip coordinates from current frame
                vector<vector<Point>> pickContours;
                Mat gray, brightGray, binary;
                cvtColor(selectedFrame, gray, COLOR_BGR2GRAY);
                add(gray, Scalar(brightnessValue), brightGray);
                threshold(brightGray, binary, thresholdValue, 255, THRESH_BINARY);

                // Mask top 10% of the frame
                int ignoreTop = static_cast<int>(0.1 * binary.rows);
                binary.rowRange(0, ignoreTop).setTo(0);

                // Mask below baseline if available
                if (fixedBaselineY != -1) {
                    binary.rowRange(static_cast<int>(fixedBaselineY), binary.rows).setTo(0);
                }

                findContours(binary, pickContours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

                vector<Point> pickTips;
                int blockWidth = 500;  // Width of each horizontal scan block

                // Scale all contour points first and collect them
                vector<Point> allPoints;
                for (const auto& contour : pickContours) {
                    for (const auto& pt : contour) {
                        allPoints.push_back(pt * 4);  // Scale to zoomed frame
                    }
                }

                // Scan in 50px horizontal blocks
                for (int xStart = 0; xStart < selectedFrame.cols * 4; xStart += blockWidth) {
                    int xEnd = xStart + blockWidth;

                    Point minYPoint(-1, INT_MAX);  // INT_MAX = very large value
                    bool found = false;

                    for (const auto& pt : allPoints) {
                        if (pt.x >= xStart && pt.x < xEnd) {
                            if (pt.y < minYPoint.y) {
                                minYPoint = pt;
                                found = true;
                            }
                        }
                    }

                    if (found) {
                        pickTips.push_back(minYPoint);
                    }
                }


                // Step 2: Match against red circles
                vector<Point> matchedTips;
                set<int> usedRedIndices;

                double maxDist = 50.0;
                for (const Point& tip : pickTips) {
                    double bestDist = maxDist;
                    int bestIndex = -1;

                    for (size_t i = 0; i < recordedRedCircles.size(); ++i) {
                        if (usedRedIndices.count(i)) continue;  // Skip already matched

                        double dist = norm(tip - recordedRedCircles[i]);
                        if (dist < bestDist) {
                            bestDist = dist;
                            bestIndex = static_cast<int>(i);
                        }
                    }

                    if (bestIndex != -1) {
                        matchedTips.push_back(tip);
                        usedRedIndices.insert(bestIndex);
                    }
                }


                // Step 3: Output and highlight
                if (!matchedTips.empty()) {
                    cout << "\n[AZIMUTH] Pick(s) in Azimuth at timeframe " << timestamp_sec << "s:\n" << endl;
                    cout << "  Azimuth Pick Count in this Frame: " << matchedTips.size() << endl;

                    outFile << "\n[AZIMUTH] Pick(s) in Azimuth timeframe " << timestamp_sec << "s:\n" << endl;
                    outFile << "  Azimuth Pick Count in this Frame: " << matchedTips.size() << endl;
                    for (const Point& tip : matchedTips) {
                        int zoomedBaselineY = fixedBaselineY * 4; // Scale baseline to match zoomed image
                        int heightAboveBaseline = zoomedBaselineY - tip.y;
                        //cout << "  Pick Tip: (" << tip.x << ", " << zoomed.rows - tip.y << ") -> Height above baseline: " << heightAboveBaseline << " px" << endl;
                        cout << "  Azimuth Pick at x-position " << tip.x << " px with Pick Height: " << heightAboveBaseline << " px" << endl;
                        outFile << "  Azimuth Pick at x-position " << tip.x << " px with Pick Height: " << heightAboveBaseline << " px" << endl;


                        // Yellow circle
                        circle(zoomed, tip, 6, Scalar(0, 255, 255), 2);


                        // Create coordinate text
                        string coordText2 = "(" + to_string(tip.x) + "," + to_string(zoomed.rows - tip.y) + ")";
                        int textX = tip.x + 5;
                        int textY = tip.y - 30;

                        if (tip.x > zoomed.cols - 100) {
                            textX = tip.x - 70;   // move left if near right edge
                            textY = tip.y - 30;   // move text below
                        }

                        putText(zoomed, coordText2, Point(textX, textY), FONT_HERSHEY_PLAIN, 0.9, Scalar(0, 255, 255), 1);

                        string heightText = "True Pick Height: " + to_string(heightAboveBaseline) + " px";
                        putText(zoomed, heightText, Point(textX, textY + 80), FONT_HERSHEY_PLAIN, 0.9, Scalar(127, 0, 255), 1);


                        // PRELIMINARY SEMI-TRIANGULAR CONTOURS

                        // Define triangle below the tip
                        int triHeight = 300;  // vertical size
                        int triWidth = 400;   // horizontal base width

                        Point pt1 = tip;  // tip of triangle
                        Point pt2(tip.x - triWidth / 2, tip.y + triHeight);
                        Point pt3(tip.x + triWidth / 2, tip.y + triHeight);

                        vector<Point> triangle{ pt1, pt2, pt3 };
                        polylines(zoomed, triangle, true, Scalar(255, 0, 255), 2);  // Pink triangle (BGR)
                    }
                }
                else {
                    cout << "\n[AZIMUTH] No picks aligned with azimuth at frame " << timestamp_sec << "s.\n";
                    outFile << "\n[AZIMUTH] No picks aligned with azimuth at frame " << timestamp_sec << "s.\n";
                }


                // ------- PRELIMINARY CONTOUR + PICK AZIMUTH MATCHING END ------- //

            }
        }

        // Process green pixels once: convert to blue or restore (azimuth contour graphic corrections)
        for (int y = 0; y < zoomed.rows; ++y) {
            for (int x = 0; x < zoomed.cols; ++x) {
                Vec3b& pixel = zoomed.at<Vec3b>(y, x);
                if (pixel == Vec3b(0, 255, 0)) {
                    bool inYrange = (y > zoomed.rows * 0.1 && y < fixedBaselineY * 4);
                    bool inXrange = (x >= 5 && x <= zoomed.cols - 8);
                    if (inYrange && inXrange) {
                        pixel = Vec3b(255, 0, 0);  // blue 
                    }
                    else {
                        pixel = originalZoomed.at<Vec3b>(y, x);  // restore
                    }
                }
            }
        }

        // Add label for azimuth contour line
        putText(zoomed, "Azimuth Contour Line", Point(10, 100), FONT_HERSHEY_SIMPLEX, 0.9, Scalar(255, 0, 0), 2);


        // ------- AZIMUTH CONTOUR BUILD END ------- //

        // ------- CUTTING LINE STARTS ------- //

        // Step 1: Extract sorted x-values from red circles
        vector<int> xValues;
        for (const auto& pt : recordedRedCircles) {
            xValues.push_back(pt.x);
        }
        sort(xValues.begin(), xValues.end());

        // Step 2: Compute adjacent spacings
        vector<int> spacings;
        for (size_t i = 1; i < xValues.size(); ++i) {
            spacings.push_back(xValues[i] - xValues[i - 1]);
        }

        // Step 3: Compute Median
        vector<int> sortedSpacings = spacings;
        sort(sortedSpacings.begin(), sortedSpacings.end());
        int median;
        size_t n = sortedSpacings.size();
        if (n % 2 == 0)
            median = (sortedSpacings[n / 2 - 1] + sortedSpacings[n / 2]) / 2;
        else
            median = sortedSpacings[n / 2];

        // Step 4: Compute MAD (Median Absolute Deviation)
        vector<int> absDeviations;
        for (int s : spacings) {
            absDeviations.push_back(abs(s - median));
        }
        sort(absDeviations.begin(), absDeviations.end());
        int mad;
        n = absDeviations.size();
        if (n % 2 == 0)
            mad = (absDeviations[n / 2 - 1] + absDeviations[n / 2]) / 2;
        else
            mad = absDeviations[n / 2];

        // Step 5: Filter spacings using MAD threshold
        vector<int> filteredSpacings;
        int threshold = 2 * mad;
        for (int s : spacings) {
            if (abs(s - median) <= threshold) {
                filteredSpacings.push_back(s);
            }
        }

        // Step 6: Compute average spacing from filtered values
        int sum = accumulate(filteredSpacings.begin(), filteredSpacings.end(), 0);
        int averageSpacing = filteredSpacings.empty() ? 0 : sum / filteredSpacings.size();

        cout << "\nFiltered Spacings: ";
        for (int s : filteredSpacings) cout << s << " ";
        cout << "\nEstimated Cutting Line Distance: " << averageSpacing << " px\n";
        outFile << "\nEstimated Cutting Line Distance: " << averageSpacing << " px\n";

        // Step 7: Draw vertical cutting lines using averageSpacing, anchored at median red circle
        if (xValues.size() >= 2 && averageSpacing > 0) {
            // Use the x-coordinate of the median red point as anchor
            int xStart = xValues[xValues.size() / 2];

            // Draw vertical dashed lines to the left of the anchor
            for (int x = xStart - averageSpacing; x >= 0; x -= averageSpacing) {
                drawDashedLine(zoomed, Point(x, 0), Point(x, zoomed.rows), Scalar(200, 200, 0));  // Yellow dashed line
                putText(zoomed, to_string(x), Point(x + 2, 20), FONT_HERSHEY_PLAIN, 0.8, Scalar(200, 200, 0), 1);
            }

            // Draw vertical dashed lines to the right of the anchor (including anchor itself)
            for (int x = xStart; x < zoomed.cols; x += averageSpacing) {
                drawDashedLine(zoomed, Point(x, 0), Point(x, zoomed.rows), Scalar(200, 200, 0));  // Yellow dashed line
                putText(zoomed, to_string(x), Point(x + 2, 20), FONT_HERSHEY_PLAIN, 0.8, Scalar(200, 200, 0), 1);
            }

            // Cutting lines now divide the frame at regular horizontal intervals
        }


        // ----- CUTTING LINE ENDS ------- //


        // ------- BASE LINE PLOT START ------- //

        // If the fixed baseline was detected earlier, draw it
        if (fixedBaselineY != -1) {
            int zoomedBaselineY = fixedBaselineY * 4; // Scale the baseline for zoomed image

            line(zoomed, Point(0, zoomedBaselineY), Point(zoomed.cols, zoomedBaselineY), Scalar(0, 255, 255), 5); // Draw yellow baseline    
            string label = "Baseline: " + to_string(zoomed.rows - zoomedBaselineY) + " px from bottom";
            putText(zoomed, label, Point(10, zoomedBaselineY - 10), FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0, 255, 255), 2);

            //cout << "Using fixed baseline at y = " << fixedBaselineY << " (zoomed y = " << zoomedBaselineY << ")" << endl;
        }
        else {
            cout << "Baseline not available!" << endl;
        }

        // ------- BASE LINE PLOT END ------- //

        // Show the zoomed image in a new window
        imshow("Resampled Frame", zoomed);

        // Save the zoomed image to disk with timestamp - PNG version
        imwrite(filename, zoomed);
        cout << "\nSaved resampled frame as: " << filename << endl;

		//// Save the zoomed image to disk with timestamp - EPS conversion
        //imwrite(filename, zoomed);
        //string epsCommand = "magick \"" + filename + "\" -density 96 eps:\"" + filename.substr(0, filename.size() - 4) + ".eps\"";
        //system(epsCommand.c_str());
        //// Delete the original PNG file
        //remove(filename.c_str());
        //
        //cout << "\nSaved resampled frame at: Outputs/" << baseName << "_" + to_string(timestamp_sec) + "s.eps" << endl;

    }
}


int main() {
    // Set the path to the video file
    //string path = "Resources/video_0.mp4";

    // Create Output folder
	system("mkdir Outputs >nul 2>&1"); // null suppresses output, 2>&1 redirects error to null

    // Ask user to select a video
    cout << "\nSelect a video to play:" << endl;
    cout << "0. video_0.mp4 (suggested tuning- brightness: 11 bin threshold: 101)" << endl;
    cout << "1. video_1.mp4 (suggested tuning- brightness: 11 bin threshold: 101)" << endl;
    cout << "2. video_2.mp4 (suggested tuning- brightness: 20 bin threshold: 80)" << endl;
    cout << "\nEnter your choice 0-2 (Default: 0): " << endl;


    int choice;
    cin >> choice;

    string path;
    switch (choice) {
    case 0:
        path = "Resources/video_0.mp4";
        break;
    case 1:
        path = "Resources/video_1.mp4";
        break;
    case 2:
        path = "Resources/video_2.mp4";
        break;
    default:
        cout << "Invalid choice. Using default video_0.mp4." << endl;
        path = "Resources/video_0.mp4";
    }

    // Extract the video base name for output naming
    size_t lastSlash = path.find_last_of("/\\");
    string fileOnly = path.substr(lastSlash + 1);
    size_t dotPos = fileOnly.find_last_of('.');
    baseName = fileOnly.substr(0, dotPos);  // "video_0"

    // Open the video file
    VideoCapture cap(path);
    if (!cap.isOpened()) {
        cerr << "Error: Cannot open video file." << endl;
        return -1;
    }

    // Create a resizable window for video playback
    namedWindow("Cutting Drum Video", WINDOW_NORMAL);

    // Track window size to detect resize
    int prevWinWidth = -1, prevWinHeight = -1;


    // Assign the mouse click handler and pass the capture object (if needed in future)
    setMouseCallback("Cutting Drum Video", onMouse);

    // Create a window for controlling contour detection
    namedWindow("Contour Adjustment Panel", WINDOW_NORMAL);
    resizeWindow("Contour Adjustment Panel", 500, 100);
    createTrackbar("Brightness", "Contour Adjustment Panel", &brightnessValue, 100);        // Range: 0-100
    createTrackbar("Bin Thresh", "Contour Adjustment Panel", &thresholdValue, 255);         // Range: 0-255

    // Main video display loop
    bool windowIsOpen = true;
    while (windowIsOpen) {
        Mat frame;

        // Read a frame from the video
        if (!cap.read(frame)) {
            // If we reached the end of the video, loop back to the start
            cap.set(CAP_PROP_POS_FRAMES, 0);
            continue;
        }

        // Clone the current frame for use in the mouse callback
        currentFrame = frame.clone();
        currentTimestamp = cap.get(CAP_PROP_POS_MSEC); // Get timestamp in milliseconds

        // ------- USER CHANGE TRACKER ------- //
        if (brightnessValue != prevBrightnessValue || thresholdValue != prevThresholdValue) {
            accumulatedBinary.release();  // Properly clears the matrix memory
            isFirstBinary = true;         // Reset the flag so next binary becomes the base

            cout << "Trackbar values changed -> clearing accumulated binary mask." << endl;

            prevBrightnessValue = brightnessValue;
            prevThresholdValue = thresholdValue;
        }


        // ------- BASELINE DETECTION PER FRAME ------- //

        // Convert current frame to grayscale and apply Gaussian blur
        Mat gray;
        cvtColor(frame, gray, COLOR_BGR2GRAY);
        GaussianBlur(gray, gray, Size(5, 5), 2);

        // Detect horizontal transitions using vertical gradient
        Mat gradY;
        Sobel(gray, gradY, CV_32F, 0, 1, 3); // 0,1 is derivative order; 0 in X (no horizontal), 1 in Y (vertical), so it detects horzontal edges, 3 is kernel standard in Sobel

        // Sum vertical edge strength per row
        Mat rowEnergy;
        reduce(abs(gradY), rowEnergy, 1, REDUCE_SUM, CV_32F); //sum absolute gradient value of every row, reduced to 1-column matrix rowEnergy

        // Only scan from middle to near bottom of image
        int startY = gray.rows / 2; // Mid-point
        int endY = gray.rows * 9 / 10; // 90% of the image height towards bottom

        float maxEnergy = 0;
        int detectedY = -1;

        // Loop over rows to find the strongest horizontal edge
        for (int y = startY; y < endY; ++y) {
            float energy = rowEnergy.at<float>(y);
            if (energy > maxEnergy) {
                maxEnergy = energy;
                detectedY = y;
            }
        }

        // If this baseline is higher (closer to top) than previous, update it
        if (detectedY != -1 && (fixedBaselineY == -1 || detectedY < fixedBaselineY)) {
            fixedBaselineY = detectedY + 2.25;
            //cout << "Updated fixed baselineY to: " << fixedBaselineY << endl;
        }

        // ------- CONTOUR DETECTION PER FRAME ------- //

        // Apply brightness by adding a scalar value
        Mat brightGray;
        add(gray, Scalar(brightnessValue), brightGray);  // Add brightness from trackbar

        // Apply binary thresholding with dynamic value
        Mat binary;
        threshold(brightGray, binary, thresholdValue, 255, THRESH_BINARY);

        // Mask out top 10% of the image
        int ignoreTop = static_cast<int>(0.1 * binary.rows);
        binary.rowRange(0, ignoreTop).setTo(0);

        // Mask out everything below baseline (if baseline is available)
        if (fixedBaselineY != -1) {
            binary.rowRange(static_cast<int>(fixedBaselineY), binary.rows).setTo(0);
        }



        // --- FOR AZIMUTH CONTOUR: Accumulate binary masks across frames ---
        if (isFirstBinary) {
            accumulatedBinary = binary.clone();
            isFirstBinary = false;
        }
        else {
            bitwise_or(accumulatedBinary, binary, accumulatedBinary);
        }
        // --- END OF AZIMUTH CONTOUR ACCUMULATION ---


        // Find contours from the masked binary image
        vector<vector<Point>> pickContours;
        findContours(binary, pickContours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);


        // Draw the contours on the original frame (for real-time display)
        drawContours(frame, pickContours, -1, Scalar(0, 255, 0), 2);


        try {
            // Detect window resize
            int winWidth = getWindowImageRect("Cutting Drum Video").width;
            int winHeight = getWindowImageRect("Cutting Drum Video").height;

            if (winWidth != prevWinWidth || winHeight != prevWinHeight) {
                cout << "Window resized or moved. Resetting accumulated binary mask." << endl;
                accumulatedBinary.release();
                isFirstBinary = true;
                prevWinWidth = winWidth;
                prevWinHeight = winHeight;
            }

            // Display the current frame in the window
            imshow("Cutting Drum Video", frame);
        }
        catch (const cv::Exception& e) {
            // Handle exceptions (e.g., if the window is closed unexpectedly)
            cerr << "OpenCV error: " << e.what() << endl;
            break;
        }

        // Wait for 30 milliseconds for a key press
        int key = waitKey(30);
        if (key == 27) break;  // ESC key pressed -> exit the loop

        // Optional: additional check to see if window was manually closed (more robust)
        //double prop = -1;
        //try {
        //    prop = getWindowProperty("Cutting Drum Video", WND_PROP_VISIBLE); // This function returns value indicating if window is visible (1 if visible, 0 if closed)
        //}
        //catch (...) {
        //    break;  // Exit if window property cannot be queried (assume closed)
        //}
        //if (prop < 1) break;  // If the window is no longer visible, exit
        try {
            int winVisible = (int)getWindowProperty("Cutting Drum Video", WND_PROP_VISIBLE);
            if (winVisible < 1) break;

            imshow("Cutting Drum Video", frame);
        }
        catch (const cv::Exception& e) {
            cerr << "Window handling exception: " << e.what() << endl;
            break;
        }

    }

    // Clean up: release video capture and destroy all OpenCV windows
    cap.release();
    destroyAllWindows();
    return 0;
}
