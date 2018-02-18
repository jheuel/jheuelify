#ifndef __FACESWAPPER_H__
#define __FACESWAPPER_H__

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

//#include <dlib/gui_widgets.h>
#include <dlib/image_io.h>
#include <dlib/image_processing.h>
#include <dlib/image_processing/frontal_face_detector.h>
//#include <dlib/image_processing/render_face_detections.h>
#include <dlib/opencv.h>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/photo.hpp>
#include <opencv2/video/tracking.hpp>
#include <random>
#include <vector>
using namespace boost::filesystem;

#include "mask.h"
#include "transformation.h"
#include <string>

using std::string;
using std::cout;
using std::endl;

// ----------------------------------------------------------------------------------------

class NoFaceFoundException : public exception {
  virtual const char *what() const throw() { return "No face found"; }
} nofacefound;

struct result {
  cv::Mat im;
  std::vector<cv::Mat> masks;
  std::vector<std::vector<cv::Point>> shapes;
};

class FaceSwapper {
  std::vector<result> input_faces;
  frontal_face_detector detector;
  shape_predictor sp;
  string output_path;
  std::random_device seeder;
  std::mt19937 rng;
  std::uniform_int_distribution<int> gen;
  result get_random_face();

public:
  FaceSwapper(string model, string replacement_faces_path, string output_path);
  result get_all_faces_from_file(string filename);

  void swap(string dest_face_path, string filename);
};

bool endsWith(std::string const &a, std::string const &b) {
  if (b.length() > a.length()) {
    return false;
  }

  return 0 == a.compare(a.length() - b.length(), b.length(), b);
}

FaceSwapper::FaceSwapper(string model, string replacement_faces_path,
                         string output_path)
    : output_path(output_path) {

  deserialize(model) >> sp;

  detector = get_frontal_face_detector();

  path p(replacement_faces_path);
  // for (auto i = directory_iterator(path(p)); i != directory_iterator(); i++)
  // {
  for (auto &i : boost::make_iterator_range(directory_iterator(p), {})) {
    cout << i << endl;
    if (endsWith(i.path().filename().string(), ".jpg")) {
      cout << i.path().filename().string() << endl;
      try {
        input_faces.push_back(get_all_faces_from_file(i.path().string()));
      } catch (exception &e) {
        cout << e.what() << endl;
      }
    }
  }

  rng = std::mt19937(seeder());
  gen = std::uniform_int_distribution<int>(0, input_faces.size() - 1);
}

result FaceSwapper::get_random_face() { return input_faces[gen(rng)]; }

void FaceSwapper::swap(string dest_faces_path, string filename) {
  result dest_faces;

  dest_faces = get_all_faces_from_file(dest_faces_path);

  Mat destination = dest_faces.im;
  cv::Size s = destination.size();
  // Mat combined_mask = cv::Mat::zeros(destination.size(), CV_8U);
  if (dest_faces.shapes.size() == 0) {
    throw nofacefound;
  }

  for (unsigned int i = 0; i < dest_faces.shapes.size(); i++) {
    auto face = get_random_face();
    cout << "face nr. " << i << endl;
    auto M = transformation(dest_faces.shapes[i], face.shapes.back());
    // auto M = cv::estimateRigidTransform(dest_faces.shapes[i],
    // face.shapes.back(), false);
    cout << "M " << M << endl;
    // cv::Size s = v[0].im.size();
    auto warped_im = warp(face.im, M, s);
    auto warped_mask = warp(face.masks.back(), M, s);

    // cv::imwrite("faces1.jpg", warped_im);
    // cv::imwrite("warped_mask1.jpg", warped_mask);

    cout << "does it die here?" << endl;
    std::vector<std::vector<Point>> contours;
    cout << "or here?" << endl;
    findContours(warped_mask, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
    if (contours.size() == 0) {
      continue;
    }
    cv::Rect roi = cv::boundingRect(contours[0]);

    cout << "roi: " << roi << endl;
    cout << "im: " << face.im.cols << " " << face.im.rows << endl;
    warped_im = warped_im(roi);
    warped_mask = warped_mask(roi);

    // cv::imwrite("dest_face.jpg", dest_face.im);
    // cv::imwrite("face.jpg", warped_im);
    // cv::imwrite(output_path + "warped_mask.jpg", warped_mask);

    // string destination_path = output_path + "destination.jpg";

    // string source_path = output_path + "source.jpg";
    string mask_path = filename.substr(0, filename.length() - 4) + "_mask.jpg";
    cout << mask_path << endl;

    // source_path = folder + "source1.png";
    // mask_path = folder + "mask.png";
    // string destination_path = folder + "destination1.png";

    Mat source = warped_im;
    // im2(roi).copyTo(source);
    // im.copyTo(destination);
    // Mat mask(warped_mask, roi);

    // Mat source = imread(source_path, IMREAD_COLOR);
    // Mat destination = imread(destination_path, IMREAD_COLOR);
    imwrite(mask_path, warped_mask);
    Mat mask = imread(mask_path, IMREAD_COLOR);
    // Mat mask = warped_mask;

    // Mat result;
    Point p;
    p.x = roi.x + int(roi.width / 2.);
    p.y = roi.y + int(roi.height / 2.);
    cout << "p: " << p << endl;
    // cout << "source size: " << source.size() << endl;
    cout << "dest size: " << destination.size() << endl;

    // source.copyTo(destination(roi), mask);
    // mask.copyTo(combined_mask(roi));

    seamlessClone(source, destination, mask, p, destination, NORMAL_CLONE);
  }

  // std::vector<std::vector<Point>> contours;
  // findContours(combined_mask, contours, CV_RETR_EXTERNAL,
  // CV_CHAIN_APPROX_NONE);
  // int minx = destination.cols;
  // int miny = destination.rows;
  // int maxx = 0;
  // int maxy = 0;

  // for (int c = 0; c < contours.size(); c++) {
  // auto r = cv::boundingRect(contours[c]);
  // if (r.x < minx) minx = r.x;
  // if (r.y < miny) miny = r.y;
  // if (r.width+r.x > maxx) maxx = r.width+r.x;
  // if (r.height+r.y > maxy) maxy = r.height+r.y;
  //}

  // cv::Rect roi(minx, miny, maxx-minx, maxy-miny);

  // Point p;
  // p.x = dest_faces.im.cols/2.;
  // p.y = dest_faces.im.rows/2.;
  // p.x = roi.x + int(roi.width / 2.);
  // p.y = roi.y + int(roi.height / 2.);
  // cout << "p: " << p << endl;
  // cout << "dest size: " << dest_faces.im.size() << endl;

  // destination = destination(roi);
  // combined_mask = combined_mask(roi);

  // imwrite("/Users/hannes/Desktop/destination.jpg", destination);
  // imwrite("/Users/hannes/Desktop/combined_mask.jpg", combined_mask);

  // cout << "may die here" << endl;
  // Mat result;
  // Mat cmask = imread("/Users/hannes/Desktop/combined_mask.jpg",
  // IMREAD_COLOR);
  // seamlessClone(destination, dest_faces.im, cmask, p, result, NORMAL_CLONE);
  // imwrite(filename, result);

  imwrite(filename, destination);
}

result FaceSwapper::get_all_faces_from_file(string filename) {
  cout << "processing image " << filename << endl;
  array2d<bgr_pixel> img;
  load_image(img, filename);

  // Make the image larger so we can detect small faces.
  pyramid_up(img);

  result res;

  auto im = cv::Mat(toMat(img));
  im.copyTo(res.im);

  // Now tell the face detector to give us a list of bounding boxes
  // around all the faces in the image.
  std::vector<dlib::rectangle> dets = detector(img);
  cout << "Number of faces detected: " << dets.size() << endl;
  if (dets.size() == 0) {
    throw nofacefound;
  }

  // Now we will go ask the shape_predictor to tell us the pose of
  // each face we detected.
  std::vector<full_object_detection> shapes;
  for (unsigned long j = 0; j < dets.size(); j++) {
    full_object_detection shape = sp(img, dets[j]);
    shapes.push_back(shape);
  }
  if (shapes.size() == 0) {
    throw nofacefound;
  }

  // Now let's view our face poses on the screen.
  // image_window win, win_faces;
  // win.clear_overlay();
  // win.set_image(cv_image<bgr_pixel>(toMat(img)));
  // win.add_overlay(render_face_detections(shapes));
  // cin.get();

  // We can also extract copies of each face that are cropped, rotated
  // upright,
  // and scaled to a standard size as shown here:
  // dlib::array<array2d<bgr_pixel> > face_chips;
  // extract_image_chips(img, get_face_chip_details(shapes), face_chips);
  // win_faces.set_image(tile_images(face_chips));

  // std::vector<cv::Point> vec(shapes[0].num_parts());

  cout << "save hulls in vector" << endl;

  for (unsigned int j = 0; j < shapes.size(); j++) {
    std::vector<cv::Point> tmp;
    for (unsigned int i = 0; i < shapes[j].num_parts(); i++) {
      auto s = shapes[j].part(i);
      tmp.push_back(cv::Point(s.x(), s.y()));
    }
    res.shapes.push_back(tmp);

    cv::Mat face_mask = cv::Mat::zeros(im.size(), CV_8U);
    drawConvexHull(face_mask, res.shapes.back());
    res.masks.push_back(face_mask);
  }

  cout << "end of run" << endl;
  return res;
}

#endif /* ifndef __FACESWAPPER_H__ */
