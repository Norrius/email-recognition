#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <tesseract/baseapi.h>
#include <highgui.h>

using namespace cv;
using namespace std;

RNG rng(12345);

vector<Mat> images;
int a = 30; // Canny threshold 1
int b = 50; // Canny threshold 2
// int g = 5; // Gauss
int d1 = 15; // Kernel size
int d2 = 1; // 
int n = 0; // image number
int m = 0; // field number

const char *WINDOW_NAME = "Display Image";

tesseract::TessBaseAPI tess;

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

char* getText(const Mat &image) {
    tess.SetImage((uchar*)image.data, image.size().width,
        image.size().height, image.channels(), image.step1());
    tess.Recognize(0);
    return tess.GetUTF8Text();
}

void onTrackbar(int, void*) {
    RotatedRect rect = getRectangle(images[n], a, b);
    Mat card = cutRect(images[n], rect);

    Mat output;
    vector<Rect> fields = morph(card, d1, d2, output);
    
    if (m >= fields.size()) {
        m = fields.size() - 1;
    }

    imshow(WINDOW_NAME, card(fields[m]));
    // cout << getText(card(fields[m]).clone());
}

vector<string> getTextFromAllFields(const Mat &image) {
    RotatedRect rect = getRectangle(image, a, b);
    Mat card = cutRect(image, rect);
    Mat output;
    vector<Rect> fields = morph(card, d1, d2, output);
    vector<string> result;
    for (Rect rect : fields) {
        result.push_back(getText(card(rect)));
    }
    return result;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "missing argument" << endl;
        return -1;
    }

    tess.Init(0, "eng");
    tess.SetVariable("tessedit_char_whitelist", "abcdefghijklmnopqrstuvwxyz012345789:-.@<>");
    
    if (strcmp(argv[1], "-cut") == 0) {
        for (int i = 2; i < argc; ++i) {
            Mat image = imread(argv[i]);
            if (!image.data) {
                cerr << "could not load image" << endl;
                return -1;
            }

            RotatedRect rect = getRectangle(image, a, b);
            Point2f rectPoints[4];
            rect.points(rectPoints);
            for (int i = 0; i < 4; ++i) {
                cout << rectPoints[i] << ", ";
            }
        }
        return 0;
    } else if (strcmp(argv[1], "-text") == 0) {
        for (int i = 2; i < argc; ++i) {
            Mat image = imread(argv[i]);
            if (!image.data) {
                cerr << "could not load image" << endl;
                return -1;
            }

            vector<string> text = getTextFromAllFields(image);
            for (String s : text) {
                cout << s << endl;
            }
        }
        return 0;
    }
    images = vector<Mat>(argc - 1);
    for (int i = 1; i < argc; ++i) {
        // Mat image;
        images[i-1] = imread(argv[i]);
        if (!images[i-1].data) {
            cerr << "could not load image" << endl;
            return -1;
        }
    }

    namedWindow(WINDOW_NAME, CV_GUI_EXPANDED);
    // setWindowProperty(WINDOW_NAME, WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
    createTrackbar("Threshold 1", WINDOW_NAME, &a, 200, onTrackbar);
    createTrackbar("Threshold 2", WINDOW_NAME, &b, 200, onTrackbar);
    createTrackbar("Kernel X", WINDOW_NAME, &d1, 15, onTrackbar);
    createTrackbar("Kernel Y", WINDOW_NAME, &d2, 15, onTrackbar);
    if (images.size() > 1) {
        createTrackbar("Image", WINDOW_NAME, &n, images.size()-1, onTrackbar);
    }
    createTrackbar("Field", WINDOW_NAME, &m, 15, onTrackbar);

    onTrackbar(0, 0);
    
    // press Esc to close
    int r = waitKey(0);
    while (r != 1048603 && r != 27) {
        r = waitKey(0);
    }

    return 0;
}
