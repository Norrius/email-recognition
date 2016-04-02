#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <tesseract/baseapi.h>
#include <highgui.h>

using namespace cv;
using namespace std;

RNG rng(12345);

Mat image;
int a = 30; // Canny threshold 1
int b = 50; // Canny threshold 2
// int g = 5; // Gauss
int d1 = 15; // Kernel size
int d2 = 1; // 
int n = 0;

const char *WINDOW_NAME = "Display Image";

tesseract::TessBaseAPI tess;

/*void showContours() {
    Mat shapes = Mat::zeros(image.size(), CV_8UC3); // draw contours here
    vector<vector<Point> > contours;
    vector<vector<Point> > good;
    vector<vector<Point> > bad;
    Mat gray;
    cvtColor(image, gray, CV_BGR2GRAY);
    GaussianBlur(gray, gray, Size(5, 5), 1, 1);
    Mat bw;
    Canny(gray, bw, a, b, 3, true);
    findContours(bw.clone(), contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
    // for (int i = 0; i < contours.size(); ++i) {
    //     Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255));
    //     drawContours(shapes, contours, i, color, 2, 8, 0, 0, Point());
    // }

    // imshow(WINDOW_NAME, shapes);

    // return;
    
    std::vector<cv::Point> approx;
    for (int i = 0; i < contours.size(); ++i) {
        // Approximate contour with accuracy proportional
        // to the contour perimeter
        approxPolyDP(
            Mat(contours[i]), 
            approx, 
            arcLength(Mat(contours[i]), true) * 0.02, 
            true
        );

        if (fabs(cv::contourArea(contours[i])) > 1000 &&
            arcLength(Mat(contours[i]), true) > 1000) {
            good.push_back(approx);
            cout << fabs(cv::contourArea(contours[i])) << ' '
                 << arcLength(Mat(contours[i]), true) << '\n';

            RotatedRect rect = minAreaRect(approx);
            Point2f vtx[4];
            rect.points(vtx);
            for (int j = 0; j < 4; ++j) {
                line(image, vtx[j], vtx[(j+1)%4], Scalar(0, 255, 0), 1, 16);
                cout << vtx[j] << endl;
            }
        } else {
            bad.push_back(approx);
        }



        // for (int j = 0; j < approx.size(); ++i) 
        // cerr << approx.size() << endl;
    }
    image.copyTo(shapes);
    for (int i = 0; i < good.size(); ++i) {
        drawContours(shapes, good, i, Scalar(255, 255, 255), 2, 8, 0, 0, Point());
    }
    for (int i = 0; i < bad.size(); ++i) {
        Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255));
        drawContours(shapes, bad, i, color, 2, 8, 0, 0, Point());
    }
    imshow(WINDOW_NAME, shapes);
}*/

bool polyComparator(const vector<Point> &a, const vector<Point> &b) {
    return fabs(contourArea(a)) < fabs(contourArea(b));
}

RotatedRect getRectangle(const Mat &image, int thr1, int thr2) {
    Mat gray;
    cvtColor(image, gray, CV_BGR2GRAY);
    GaussianBlur(gray, gray, Size(5, 5), 1, 1);

    Mat canny_output;
    Canny(gray, canny_output, thr1, thr2, 3, true);
    
    vector<vector<Point> > contours;
    findContours(canny_output, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

    vector<vector<Point> > polys(contours.size());
    for (int i = 0; i < (int)contours.size(); ++i) {
        approxPolyDP(Mat(contours[i]), polys[i], 
                     arcLength(Mat(contours[i]), true) * 0.02, true);
    }
    vector<Point> bestPoly = *max_element(polys.begin(), polys.end(), polyComparator);
    RotatedRect bestRect = minAreaRect(bestPoly);
    
    return bestRect;
}

Mat cutRect(const Mat &source, const RotatedRect &rect) {
    float angle = rect.angle;
    Size rect_size = rect.size;
    
    if (rect.angle < -45.0) {
        angle += 90.0;
        swap(rect_size.width, rect_size.height);
    }

    Mat M;
    M = getRotationMatrix2D(rect.center, angle, 1.0);
    Mat rotated;
    warpAffine(source, rotated, M, source.size(), INTER_CUBIC);
    Mat result;
    getRectSubPix(rotated, rect_size, rect.center, result);
    return result;
}

vector<Rect> morph(const Mat &image, int d1, int d2, Mat &output) {
    Mat gray;
    cvtColor(image, gray, CV_BGR2GRAY);
    Mat grad;
    Mat morphKernel = getStructuringElement(MORPH_ELLIPSE, Size(3, 3));
    morphologyEx(gray, grad, MORPH_GRADIENT, morphKernel);
    
    Mat bw;
    threshold(grad, bw, 0.0, 255.0, THRESH_BINARY | THRESH_OTSU);
    
    Mat connected;
    morphKernel = getStructuringElement(MORPH_RECT, Size(d1, d2));
    morphologyEx(bw, connected, MORPH_CLOSE, morphKernel);
    
    Mat mask = Mat::zeros(bw.size(), CV_8UC1);
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    findContours(connected, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
    
    vector<Rect> fields;
    image.copyTo(output);
    for (int i = 0; i >= 0; i = hierarchy[i][0]) {
        Rect rect = boundingRect(contours[i]);
        Mat maskROI(mask, rect);
        maskROI = Scalar(0, 0, 0);
        
        drawContours(mask, contours, i, Scalar(255, 255, 255), CV_FILLED);
        
        double r = (double)countNonZero(maskROI)/(rect.width*rect.height);

        if (r > 0.1 && (rect.height > 5 && rect.width > 10)) {
            rectangle(output, rect, Scalar(0, 0, 255), 2);
            fields.push_back(rect);
        }
    }
    // imshow(WINDOW_NAME, output);
    return fields;
}

/* display the rotatedrect on the image
    image.copyTo(output);

    Point2f rectPoints[4];
    rect.points(rectPoints);
    for (int i = 0; i < 4; ++i) {
        cout << rectPoints[i] << '\n';
        line(output, rectPoints[i], rectPoints[(i+1)%4], Scalar(0, 0, 255), 3, 8);
    }
    // cout << endl;
*/

char* getText(const Mat &image) {
    tess.SetImage((uchar*)image.data, image.size().width,
        image.size().height, image.channels(), image.step1());
    tess.Recognize(0);
    return tess.GetUTF8Text();
}

void onTrackbar(int, void*) {
    RotatedRect rect = getRectangle(image, a, b);
    Mat card = cutRect(image, rect);

    // showContours();
    Mat output;
    vector<Rect> fields = morph(card, d1, d2, output);
    /*for (int i = 0; i < fields.size(); ++i) {
        // cout << fields[i].size() << endl;
        // cout << getText(image(fields[i]))<< endl;
        cout << getText(card(fields[i]).clone());
    }*/
    // getText(card(fields[n]));

    imshow(WINDOW_NAME, card(fields[n]));
    cout << getText(card(fields[n]).clone());
}

int main(int argc, char** argv) {
    // load image
    if (argc < 2) {
        cerr << "missing argument" << endl;
        return -1;
    }

    // Mat image;
    image = imread(argv[1]);
    if (!image.data) {
        cerr << "could not load image" << endl;
        return -1;
    }

    if (argc == 3 && strcmp(argv[2], "-s") == 0) {
        // silent
        RotatedRect rect = getRectangle(image, a, b);
        Point2f rectPoints[4];
        rect.points(rectPoints);
        for (int i = 0; i < 4; ++i) {
            cout << rectPoints[i] << ", ";
            //line(output, rectPoints[i], rectPoints[(i+1)%4], Scalar(0, 0, 255), 3, 8);
        }
        return 0;
    }

    tess.Init(0, "eng");

    namedWindow(WINDOW_NAME, CV_GUI_EXPANDED);
    // setWindowProperty(WINDOW_NAME, WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);

    onTrackbar(0, 0);

    createTrackbar("Threshold 1", WINDOW_NAME, &a, 200, onTrackbar);
    createTrackbar("Threshold 2", WINDOW_NAME, &b, 200, onTrackbar);
    createTrackbar("Kernel X", WINDOW_NAME, &d1, 15, onTrackbar);
    createTrackbar("Kernel Y", WINDOW_NAME, &d2, 15, onTrackbar);
    createTrackbar("N", WINDOW_NAME, &n, 15, onTrackbar);

    // press Esc to close
    int r = waitKey(0);
    while (r != 1048603 && r != 27) {
        r = waitKey(0);
    }
    return 0;
}