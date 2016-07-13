#include "EmailRecognition.h"

using namespace cv;
using namespace std;

RNG rng(12345);

tesseract::TessBaseAPI tess;

bool polyComparator(const vector<Point> &a, const vector<Point> &b) {
    return fabs(contourArea(a)) < fabs(contourArea(b));
}

RotatedRect getRectangle(const Mat &image, const ERSettings &set, Mat *output) {
    Mat gray;
    cvtColor(image, gray, CV_BGR2GRAY);
    GaussianBlur(gray, gray, Size(9, 9), 0);

    Mat canny_output;
    Canny(gray, canny_output, set.thr1, set.thr2, 3, true);
    // canny_output.copyTo(*output);
    
    vector<vector<Point> > contours;
    findContours(canny_output, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

    vector<vector<Point> > polys(contours.size());
    for (int i = 0; i < (int)contours.size(); ++i) {
        // approxPolyDP(Mat(contours[i]), polys[i], 
                     // arcLength(Mat(contours[i]), true) * 0.02, true);
        convexHull(Mat(contours[i]), polys[i], false);
    }
    vector<Point> bestPoly = *max_element(polys.begin(), polys.end(), polyComparator);
    RotatedRect bestRect = minAreaRect(bestPoly);
    
    if (output) {
        cvtColor(gray, *output, CV_GRAY2BGR);
        // gray.copyTo(*output);
//        cerr << polys.size() << endl;
        for (int i = 0; i < (int)polys.size(); ++i) {
            Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255));
            drawContours(*output, polys, i, color, set.w, 8, 0, 0, Point());
        }
    }

    return bestRect;
}

Mat cutRect(const Mat &source, const RotatedRect &rect) {
    float angle = rect.angle;
    Size rect_size = rect.size;
    
    if (rect_size.width < rect_size.height) { // rect.angle < -45.0
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

vector<Rect> morph(const Mat &image, const ERSettings &set, Mat *output) {
    Mat gray;
    cvtColor(image, gray, CV_BGR2GRAY);
    Mat grad;
    Mat morphKernel = getStructuringElement(MORPH_ELLIPSE, Size(3, 3));
    morphologyEx(gray, grad, MORPH_GRADIENT, morphKernel);
    
    Mat bw;
    threshold(grad, bw, 0.0, 255.0, THRESH_BINARY | THRESH_OTSU);
    
    Mat connected;
    morphKernel = getStructuringElement(MORPH_RECT, Size(set.d1, set.d2));
    morphologyEx(bw, connected, MORPH_CLOSE, morphKernel);
    
    Mat mask = Mat::zeros(bw.size(), CV_8UC1);
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    findContours(connected, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
    
    vector<Rect> fields;
    if (output) {
        image.copyTo(*output);
    }
    for (int i = 0; i >= 0; i = hierarchy[i][0]) {
        Rect rect = boundingRect(contours[i]);
        Mat maskROI(mask, rect);
        maskROI = Scalar(0, 0, 0);
        
        drawContours(mask, contours, i, Scalar(255, 255, 255), CV_FILLED);
        
        double r = (double)countNonZero(maskROI)/(rect.width*rect.height);

        if (r > 0.1 && (rect.height > 5 && rect.width > 10)) {
            if (output) {
                rectangle(*output, rect, Scalar(0, 0, 255), set.w);
            }
            fields.push_back(rect);
        }
    }

    return fields;
}

char* getText(const Mat &image) {
    Mat gray;
    cvtColor(image, gray, CV_BGR2GRAY);
    Mat binary;
    threshold(gray, binary, 0.0, 255.0, CV_THRESH_BINARY | CV_THRESH_OTSU);

    tess.SetImage((uchar*)binary.data, binary.size().width,
        binary.size().height, binary.channels(), binary.step1());
    tess.Recognize(0);

    char* s = tess.GetUTF8Text();
//    cerr << tess.MeanTextConf() << endl;

    return s;
}

vector<string> getTextFromAllFields(const Mat &image, const ERSettings &set) {
    RotatedRect rect = getRectangle(image, set);
    Mat card = cutRect(image, rect);
    vector<Rect> fields = morph(card, set);
    vector<string> result;
    for (Rect rect : fields) {
        result.push_back(getText(card(rect)));
    }
    return result;
}

string chooseBestString(const vector<string> &strings) {
    int n = (int)strings.size();
    int l;
    vector<int> scores(n, 0);
    for (int i = 0; i < n; ++i) {
        if (find(strings[i].begin(), strings[i].end(), '@') != strings[i].end()) {
            scores[i] += 3;
        }
        l = strings[i].length();
        if (l > 10 && l < 25) {
            scores[i] += 1;
        }
    }
    return strings[max_element(scores.begin(), scores.end()) - scores.begin()];
}

void init() {
    tess.Init(0, "eng");
    tess.SetVariable("tessedit_char_whitelist", "abcdefghijklmnopqrstuvwxyz0123456789:-.@<>()");
}
