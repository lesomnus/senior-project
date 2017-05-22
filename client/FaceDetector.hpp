#pragma once

#include <vector>
#include <opencv2/opencv.hpp>


class FaceDetector{
public:
	FaceDetector():_scale_factor(1.1), _min_neighbors(2){
		_classifier.load("./haarcascade_frontalface_default.xml");
		_detected.reserve(8);
	}

	void set_sacle_factor(double val = 1.1){ _scale_factor = val; }
	void set_min_neighbors(int val = 2){ _min_neighbors = val; }
	void set_min_size(cv::Size val = cv::Size()){ _min_size = val; }
	void set_max_size(cv::Size val = cv::Size()){ _max_size = val; }

	const std::vector<cv::Rect>& operator()(cv::Mat& src){
		_classifier.detectMultiScale(src, _detected,
									 _scale_factor,
									 _min_neighbors, 0,
									 _min_size, _max_size);
		return _detected;
	}
private:
	std::vector<cv::Rect> _detected;
	cv::CascadeClassifier _classifier;
	double _scale_factor;
	int _min_neighbors;
	cv::Size _min_size;
	cv::Size _max_size;
};