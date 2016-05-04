#include "EmailRecognition.h"
using namespace cv;
using namespace std;

const char *WINDOW_MAIN = "Field Examination";
const char *WINDOW_CONTOUR = "Contour Detection";
const char *WINDOW_CARD = "Card Isolation";
const char *WINDOW_MORPH = "Morphology";

ERSettings set;
int n = 0; // image number
int m = 0; // field number

vector<Mat> images;


void onTrackbar(int, void*) {
    Mat outputContour;
    Mat outputCard;
    Mat outputMorph;

    RotatedRect rect = getRectangle(images[n], set, &outputContour);
    Mat card = cutRect(images[n], rect);

    vector<Rect> fields = morph(card, set, &outputMorph);
    
    if (m >= fields.size()) {
        m = fields.size() - 1;
    }

    // Main output
    Mat gray;
    cvtColor(card(fields[m]), gray, CV_BGR2GRAY);
    Mat binary;
    threshold(gray, binary, 0.0, 255.0, CV_THRESH_BINARY | CV_THRESH_OTSU);

    imshow(WINDOW_MAIN, binary);
    cout << getText(card(fields[m]).clone());

    // Show contours
    imshow(WINDOW_CONTOUR, outputContour);

    // Detected card boundaries
    images[n].copyTo(outputCard);
    Point2f rectPoints[4];
    rect.points(rectPoints);
    for (int i = 0; i < 4; ++i) {
        line(outputCard, rectPoints[i], rectPoints[(i+1)%4], Scalar(0, 0, 255), set.w, 8);
    }
    imshow(WINDOW_CARD, outputCard);

    // Morphology
    imshow(WINDOW_MORPH, outputMorph);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "missing argument" << endl;
        return -1;
    }

    init();
    
    if (strcmp(argv[1], "-cut") == 0) {
        for (int i = 2; i < argc; ++i) {
            Mat image = imread(argv[i]);
            if (!image.data) {
                cerr << "could not load image" << endl;
                return -1;
            }

            RotatedRect rect = getRectangle(image, set);
            Point2f rectPoints[4];
            rect.points(rectPoints);
            for (int i = 0; i < 4; ++i) {
                cout << rectPoints[i].x << ' ' << rectPoints[i].y << '\n';
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

            vector<string> text = getTextFromAllFields(image, set);
            string result = chooseBestString(text);
            cout << result << endl;
        }
        return 0;
    }
    images = vector<Mat>(argc - 1);
    for (int i = 1; i < argc; ++i) {
        // Mat image;
        images[i-1] = imread(argv[i]);
        // resize(images[i-1], images[i-1], Size(), 0.3, 0.3);
        if (!images[i-1].data) {
            cerr << "could not load image" << endl;
            return -1;
        }
    }

    namedWindow(WINDOW_CONTOUR, CV_GUI_EXPANDED);
    resizeWindow(WINDOW_CONTOUR, 700, 500);
    namedWindow(WINDOW_CARD, CV_GUI_EXPANDED);
    resizeWindow(WINDOW_CARD, 700, 500);
    namedWindow(WINDOW_MORPH, CV_GUI_EXPANDED);
    resizeWindow(WINDOW_MORPH, 700, 500);
    namedWindow(WINDOW_MAIN, CV_GUI_EXPANDED);
    resizeWindow(WINDOW_MAIN, 500, 500);

    createTrackbar("Threshold 1", WINDOW_MAIN, &set.thr1, 200, onTrackbar);
    createTrackbar("Threshold 2", WINDOW_MAIN, &set.thr2, 200, onTrackbar);
    createTrackbar("Kernel X", WINDOW_MAIN, &set.d1, 15, onTrackbar);
    createTrackbar("Kernel Y", WINDOW_MAIN, &set.d2, 15, onTrackbar);
    if (images.size() > 1) {
        createTrackbar("Image", WINDOW_MAIN, &n, images.size()-1, onTrackbar);
    }
    createTrackbar("Field", WINDOW_MAIN, &m, 15, onTrackbar);

    onTrackbar(0, 0);
    
    // press Esc to close
    int r = waitKey(0);
    while (r != 1048603 && r != 27) {
        r = waitKey(0);
    }

    return 0;
}
