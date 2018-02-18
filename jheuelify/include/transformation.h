#ifndef __TRANSFORMATION_H__
#define __TRANSFORMATION_H__
#include <Eigen/Core>
#include <Eigen/Eigen>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/photo.hpp>
#include <vector>

using namespace std;
using namespace cv;

std::tuple<Eigen::MatrixXd, Eigen::Vector2d, Eigen::Vector2d>
get_matrix(std::vector<cv::Point> shape) {
  double averagex = 0;
  double averagey = 0;
  double std_errx = 0;
  double std_erry = 0;
  Eigen::MatrixXd Mshape(shape.size(), 2);
  for (unsigned int i = 0; i < shape.size(); i++) {
    Mshape(i, 0) = shape[i].x;
    Mshape(i, 1) = shape[i].y;

    averagex += shape[i].x;
    averagey += shape[i].y;
  }
  averagex /= shape.size();
  averagey /= shape.size();

  for (unsigned int i = 0; i < shape.size(); i++) {
    std_errx += pow(abs(shape[i].x - averagex), 2);
    std_erry += pow(abs(shape[i].y - averagey), 2);
  }
  std_errx = sqrt(std_errx / shape.size());
  std_erry = sqrt(std_erry / shape.size());

  for (int i = 0; i < Mshape.rows(); i++) {
    Mshape(i, 0) -= averagex;
    Mshape(i, 1) -= averagey;

    Mshape(i, 0) /= std_errx;
    Mshape(i, 1) /= std_erry;
  }

  Eigen::Vector2d average;
  average << averagex, averagey;

  Eigen::Vector2d std_err;
  std_err << std_errx, std_erry;
  return std::make_tuple(Mshape, average, std_err);
}

Eigen::Vector2d calculate_face_vector(std::vector<Point> shape) {
  Eigen::Vector2d top = {0, 0};
  int ntop = 0;

  for (int i = 17; i < 27; i++, ntop++) {
    top(0) += shape[i].x;
    top(1) += shape[i].y;
  }
  for (int i = 36; i < 48; i++, ntop++) {
    top(0) += shape[i].x;
    top(1) += shape[i].y;
  }
  top(0) /= ntop;
  top(1) /= ntop;

  int nbot = 0;
  Eigen::Vector2d bot = {0, 0};
  for (int i = 48; i < 61; i++, nbot++) {
    bot(0) += shape[i].x;
    bot(1) += shape[i].y;
  }
  bot(0) /= nbot;
  bot(1) /= nbot;

  return top - bot;
}

cv::Mat transformation(std::vector<cv::Point> shape1,
                       std::vector<cv::Point> shape2) {
  cout << "generate trafo" << endl;
  cout << "size shape1 " << shape1.size() << endl;
  cout << "size shape2 " << shape2.size() << endl;

  Eigen::MatrixXd M1, M2;
  Eigen::Vector2d average1, average2, std_err1, std_err2;
  std::tie(M1, average1, std_err1) = get_matrix(shape1);
  std::tie(M2, average2, std_err2) = get_matrix(shape2);

  auto combined = M1.transpose() * M2;

  Eigen::JacobiSVD<Eigen::MatrixXd> svd(
      combined, Eigen::ComputeThinU | Eigen::ComputeThinV);

  Eigen::MatrixXd U = svd.matrixU();
  Eigen::MatrixXd V = svd.matrixV();
  Eigen::VectorXd S = svd.singularValues();
  // Eigen::MatrixXd R = (U.cwiseProduct(V)).transpose();
  Eigen::MatrixXd R = (U * V).transpose();

  cout << "U " << U << endl
       << "V " << V << endl
       << "S " << S << endl
       << "R " << R << endl;

  Eigen::Vector2d s21;
  s21(0) = std_err2(0) / std_err1(0);
  s21(1) = std_err2(1) / std_err1(1);

  cout << "s21 " << s21 << endl;

  Eigen::MatrixXd s21R(2, 2);
  // s21R << s21(0) * R(0, 0), s21(1) * R(0, 1), // hi clang-autoformat
  // s21(0) * R(1, 0), s21(1) * R(1, 1);
  s21R << s21(0) * R(0, 0), s21(1) * R(0, 1), // hi clang-autoformat
      s21(0) * R(1, 0), s21(1) * R(1, 1);
  cout << "s21R " << s21R << endl;
  // s21R.transposeInPlace();

  auto s_elems = average2 - s21R * average1;

  auto f1 = calculate_face_vector(shape1);
  auto f2 = calculate_face_vector(shape2);

  double angle = atan2(f1(0) * f2(1) - f2(0) * f1(1), f1.dot(f2));

  cout << "angle between face vectors is " << angle << endl;

  auto f = f1;
  f = s21R * f1;
  angle = atan2(f(0) * f2(1) - f2(0) * f(1), f.dot(f2));
  cout << "angle between face vectors after trafo is " << angle << endl;

  if (abs(angle) > 0.1) {
    Eigen::Matrix<double, 2, 2> rotation;
    rotation << cos(angle), -sin(angle), // hi clang-autoformat
        sin(angle), cos(angle);
    s21R = rotation * s21R;
    f = s21R * f1;
    angle = atan2(f(0) * f2(1) - f2(0) * f(1), f.dot(f2));
    cout << "angle between face vectors after trafo is now " << angle << endl;
  }

  cv::Mat res(2, 3, CV_32FC1);

  // Map the OpenCV matrix with Eigen:
  Eigen::Map<
      Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>
  A(res.ptr<float>(), res.rows, res.cols);
  A << s21R(0, 0), s21R(0, 1), s_elems(0), // hi clang-autoformat
      s21R(1, 0), s21R(1, 1), s_elems(1);

  cout << "A:" << endl << A << endl << endl;

  return res;
}

cv::Mat warp(cv::Mat im, cv::Mat M, cv::Size s) {
  cout << "warp" << endl;
  // cv::Mat output_im(im);
  cv::Mat output_im = cv::Mat::zeros(s, CV_8U);
  cout << "cv::warpAffine" << endl;
  cv::warpAffine(im, output_im, M, s, cv::WARP_INVERSE_MAP,
                 // cv::INTER_LINEAR,
                 cv::BORDER_TRANSPARENT);
  cout << "cv::warpAffine end" << endl;

  return output_im;
}

#endif /* ifndef __TRANSFORMATION_H__ */
