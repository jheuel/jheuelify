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

    // im2(roi).copyTo(source);
    // im.copyTo(destination);
    // Mat mask(warped_mask, roi);

    // Mat source = imread(source_path, IMREAD_COLOR);
    // Mat destination = imread(destination_path, IMREAD_COLOR);
    //imwrite(mask_path, warped_mask);
    //Mat read_mask = imread(mask_path, CV_8U);
    //Mat read_mask = imread(mask_path, IMREAD_COLOR);
    //auto mask = warped_mask.clone();
    //cv::Mat mask;
    //cv::Mat in[] = { warped_mask, warped_mask, warped_mask };
    //cv::merge(in, 3, mask);

    //cv::Mat mask = cv::Mat(warped_mask);
    //cv::Mat mask = cv::Mat::zeros(warped_mask.size(), CV_8U);
    //cv::Mat mask = warped_mask.clone();
    //cv::Mat mask = 0 * cv::Mat::ones(source.rows, source.cols, source.depth());
    //cv::Mat mask = read_mask.clone();
    //cv::Mat mask = cv::Mat::zeros(warped_mask.rows, warped_mask.cols, warped_mask.depth());
    //cv::Mat mask = warped_mask;
    //cv::Mat mask = cv::Mat::zeros(s, CV_8U);
    {
        // accept only char type matrices
		CV_Assert(warped_mask.depth() == CV_8U);

		int channels = warped_mask.channels();

		int nRows = warped_mask.rows;
		int nCols = warped_mask.cols * channels;

		if (warped_mask.isContinuous())
		{
			nCols *= nRows;
			nRows = 1;
		}

		int i, j;
		uchar* p;
		for(i = 0; i < nRows; ++i)
		{
			p = warped_mask.ptr<uchar>(i);
			for (j = 0; j < nCols; ++j)
			{
				if (p[j]) {
					p[j] = 255;
				}
			}
		}
    }

    Mat source = warped_im;
	Mat mask = warped_mask;
    //{
        //// accept only char type matrices
        //CV_Assert(mask.depth() == CV_8U);

        //const int channels = mask.channels();
        //const int from_channels = read_mask.channels();
        //CV_Assert(channels == from_channels);

        //MatIterator_<uchar> it;
        //MatIterator_<uchar> from_it;
        //MatIterator_<uchar> from_end;
        //MatIterator_<uchar> read_it;
        //for(read_it = read_mask.begin<uchar>(), from_it = warped_mask.begin<uchar>(),
                //it = mask.begin<uchar>(), from_end = warped_mask.end<uchar>();
                //from_it != from_end; ++it, ++from_it, ++read_it) {
            ////if (int(*from_it) > 230) {
                ///[>it = uchar(230);
            ////} else {
            //*it = *from_it;
            ///[>it = *read_it;
            ////}
            //if (*read_it != *from_it) {
                //cout << int(*from_it) << "   " << int(*read_it) << endl;
            //}
        //}
    //}


    //cout << "read_mask" << endl;
    //cout << "flags\t"  << read_mask.flags << endl;
    //cout << "type\t"  << read_mask.type() << endl;
    //cout << "rows\t"  << read_mask.rows << endl;
    //cout << "cols\t"  << read_mask.cols << endl;
    //cout << "dims\t"  << read_mask.dims << endl;

    //cout << "mask" << endl;
    //cout << "flags\t"  << mask.flags << endl;
    //cout << "type\t"  << mask.type() << endl;
    //cout << "rows\t"  << mask.rows << endl;
    //cout << "cols\t"  << mask.cols << endl;
    //cout << "dims\t"  << mask.dims << endl;

    // Mat result;
    Point p;
    p.x = roi.x + int(roi.width / 2.);
    p.y = roi.y + int(roi.height / 2.);
    cout << "p: " << p << endl;
    // cout << "source size: " << source.size() << endl;
    cout << "source size: " << source.size() << endl;
    cout << "dest size: " << destination.size() << endl;
    cout << "mask size: " << mask.size() << endl;
    cout << "mask type: " << mask.type() << endl;

     //source.copyTo(destination(roi), mask);
     //mask.copyTo(combined_mask(roi));

    Mat dest;

    seamlessClone(source, destination, mask, p, dest, NORMAL_CLONE);
    //seamlessClone(source, destination, mask, p, dest, NORMAL_CLONE);
    destination = dest;
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
