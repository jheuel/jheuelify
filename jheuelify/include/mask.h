#ifndef __MASK_H__
#define __MASK_H__

#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/photo.hpp>
#include <vector>

using namespace dlib;
using namespace std;
using namespace cv;

// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------

cv::Mat &maxoutnonzero(Mat &I) {
  // accept only char type matrices
  CV_Assert(I.depth() == CV_8U);

  int channels = I.channels();

  int nRows = I.rows;
  int nCols = I.cols * channels;

  if (I.isContinuous()) {
    nCols *= nRows;
    nRows = 1;
  }

  int i, j;
  uchar *p;
  for (i = 0; i < nRows; ++i) {
    p = I.ptr<uchar>(i);
    for (j = 0; j < nCols; ++j) {
      if (p[j] > 0) {
        p[j] = 255;
      }
    }
  }
  return I;
}

// ----------------------------------------------------------------------------------------

void drawConvexHull(cv::Mat &im, std::vector<cv::Point> vec) {
  // right brow: 17-22
  // left brow:  22-27
  // nose:       27-35
  // right eye:  36-42
  // left eye:   42-48
  // mouth:      48-61

  std::vector<cv::Point> points;

  const bool only_parts = true;
  if (only_parts) {
    std::vector<cv::Point> nose_mouth;
    for (int i = 27; i < 35; i++) {
      nose_mouth.push_back(vec[i]);
    }
    for (int i = 48; i < 61; i++) {
      nose_mouth.push_back(vec[i]);
    }
    cv::convexHull(nose_mouth, points);
    cv::fillConvexPoly(im, points, cv::Scalar(255, 255, 255)); //, 16, 0);

    cv::GaussianBlur(im, im, cv::Size(41, 41), 0);

    maxoutnonzero(im);

    std::vector<cv::Point> eyes;
    for (int i = 17; i < 22; i++) {
      eyes.push_back(vec[i]);
    }
    for (int i = 22; i < 27; i++) {
      eyes.push_back(vec[i]);
    }
    for (int i = 36; i < 42; i++) {
      eyes.push_back(vec[i]);
    }
    for (int i = 42; i < 48; i++) {
      eyes.push_back(vec[i]);
    }
    cv::convexHull(eyes, points);
    cv::fillConvexPoly(im, points, cv::Scalar(255, 255, 255)); //, 16, 0);

  } else {
    cv::convexHull(vec, points);
    cv::fillConvexPoly(im, points, cv::Scalar(255, 255, 255)); //, 16, 0);
  }

  // cv::convexHull(vec, points);
  // cv::fillConvexPoly(im, points, cv::Scalar(1, 1, 1), 16, 0);

  for (int i = 0; i < 1; i++) {
    cv::GaussianBlur(im, im, cv::Size(31, 31), 0);

    maxoutnonzero(im);
  }

  cv::GaussianBlur(im, im, cv::Size(11, 11), 0);

  // sharp edges
  // maxoutnonzero(im);
}

#endif /* ifndef __MASK_H__ */
