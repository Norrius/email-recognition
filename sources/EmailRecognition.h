#pragma once

#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <tesseract/baseapi.h>
#include <highgui.h>

struct ERSettings {
    int thr1 = 30; // Canny threshold 1
    int thr2 = 50; // Canny threshold 2
    // int g = 5; // Gauss
    int d1 = 15; // Mortph kernel size
    int d2 = 1;  // 
    int w = 5; // line width;
};

cv::RotatedRect getRectangle(const cv::Mat &image, const ERSettings &set, cv::Mat *output=0);
cv::Mat cutRect(const cv::Mat &source, const cv::RotatedRect &rect);
std::vector<cv::Rect> morph(const cv::Mat &image, const ERSettings &set, cv::Mat *output=0);
char* getText(const cv::Mat &image);
std::vector<std::string> getTextFromAllFields(const cv::Mat &image, const ERSettings &set);
std::string chooseBestString(const std::vector<std::string> &strings);

void init();
