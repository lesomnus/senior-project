#pragma once

#include <vector>
#include <opencv2/opencv.hpp>


class FaceDetector{
public:
	FaceDetector():_scale_factor(1.1), _min_neighbors(2),
		_min_size_ratio(0), _max_size_ratio(0), _last_width(0){
		_face_classifier.load("./haarcascade_frontalface_default.xml");
		_prfl_classifier.load("./haarcascade_profileface.xml");
	}

	void set_sacle_factor(double val = 1.1){ _scale_factor = val; }
	void set_min_neighbors(int val = 2){ _min_neighbors = val; }
	void set_min_size(cv::Size val = cv::Size()){ _min_size = val; }
	void set_max_size(cv::Size val = cv::Size()){ _max_size = val; }
	void set_min_size_ratio(double val = 0){ _min_size_ratio = val; }
	void set_max_size_ratio(double val = 0){ _max_size_ratio = val; }

	std::vector<cv::Rect> operator()(const cv::Mat& src){
		return _detect(src);
	}
	std::vector<cv::Rect> operator()(const cv::Mat& src,
									 const cv::Mat& background){
		_detect_not_background(src, background);
	}
private:
	cv::Mat _grayed;
	cv::CascadeClassifier _face_classifier;
	cv::CascadeClassifier _prfl_classifier;
	cv::CascadeClassifier _leye_classifier;
	cv::CascadeClassifier _reye_classifier;
	double _scale_factor;
	int _min_neighbors;
	cv::Size _min_size;
	cv::Size _max_size;
	double _min_size_ratio;
	double _max_size_ratio;

	size_t _last_width;

	cv::Size _calc_size(const cv::Size& src, double ratio){
		return cv::Size(std::floor(src.width * ratio),
						std::floor(src.width * ratio));
	}

	std::vector<cv::Rect> _detect(const cv::Mat& src){
		using namespace cv;
		std::vector<cv::Rect> rst; rst.reserve(8);
		cvtColor(src, _grayed, CV_BGR2GRAY);
		resize(_grayed, _grayed, src.size() * 2, 0, 0, INTER_CUBIC);
		equalizeHist(_grayed, _grayed);
		if(_last_width != _grayed.size().width){
			_last_width = _grayed.size().width;
			if(_min_size_ratio > 0) set_min_size(_calc_size(_grayed.size(), _min_size_ratio));
			if(_max_size_ratio > 0) set_max_size(_calc_size(_grayed.size(), _max_size_ratio));
		}

		_face_classifier.detectMultiScale(src, rst,
										  _scale_factor,
										  _min_neighbors, 0,
										  _min_size, _max_size);
		if(rst.size() > 0) return std::move(rst);

		_prfl_classifier.detectMultiScale(src, rst,
										  _scale_factor,
										  _min_neighbors, 0,
										  _min_size, _max_size);

		return std::move(rst);
	}

	std::vector<cv::Rect> _detect_not_background(const cv::Mat& src, const cv::Mat& bgr){
		using namespace cv;
		auto detected = _detect(src);
		if(detected.empty()) return detected;

		std::vector<cv::Rect> rst;
		Mat src_ = src.clone();
		Mat bgr_; resize(bgr, bgr_, src_.size());

		return rst;
	}
};