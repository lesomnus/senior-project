#pragma once

#include <cmath>
#include <opencv2/opencv.hpp>

class ForegroundDetector{
public:
	ForegroundDetector():
		do_blur_proc(true){
		using namespace cv;
		set_window(Size(16, 8));
		_erod_kernel = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
		_dila_kernel = getStructuringElement(MORPH_ELLIPSE, Size(20, 20));
	};

	// if you have low performance camera... set true...
	bool do_blur_proc;

	void set_window(const cv::Size& size){
		_window.width = size.width;
		_window.height = size.height;
		_window_pad = size - cv::Size(1, 1);
		_window_spad.width = std::floor(_window_pad.width);
		_window_spad.height = std::floor(_window_pad.height);
		_window_area = _window.area();
	}
	void set_size(const cv::Size& size){
		_src_s = size;
	}

	cv::Mat operator() (const cv::Mat& src,
						const cv::Mat& background){
		return _proc(src, background);
	}

private:
	cv::Size	_src_s;
	cv::Mat		_last_mask;
	cv::Mat		_last_masked_shot;

	cv::Rect	_window;
	cv::Size2i	_window_pad;
	cv::Size2i	_window_spad;
	int			_window_area;

	cv::Mat		_erod_kernel;
	cv::Mat		_dila_kernel;

	cv::Mat _proc(const cv::Mat& src,
				  const cv::Mat& bgr){
		using namespace cv;
		Mat bgr_; resize(bgr, bgr_, _src_s);
		Mat src_;
		// assume no feed-back environment
		if(true || _last_mask.empty()) src_ = src.clone();
		else{
			Mat src_reverse_masked;
			src.copyTo(src_reverse_masked, _last_mask);
			Mat src_masked = src - src_reverse_masked;

			src_ = src_reverse_masked
				+ (src_masked * 0.5)
				+ (_last_masked_shot * 0.5);

		}

		if(do_blur_proc) GaussianBlur(bgr_, bgr_, Size(5, 5), 1.5);

		Mat src_y, bgr_y;
		{
			cvtColor(src_, src_y, CV_BGR2YCrCb);
			cvtColor(bgr_, bgr_y, CV_BGR2YCrCb);
			extractChannel(src_y, src_y, 0);
			extractChannel(bgr_y, bgr_y, 0);
		}

		Mat rst(_src_s, CV_8UC1); rst = Scalar(0xFF);
		Mat src_trg, bgr_trg;
		Scalar src_m, src_std, bgr_m, bgr_std;
		double covar = 0, correl = 0;

		_window.x = 0; _window.y = 0;
		for(auto y = 0; y < _src_s.height - _window_pad.height; ++y){
			for(auto x = 0; x < _src_s.width - _window_pad.width; ++x){
				_window.x = x; _window.y = y;
				src_trg = src_y(_window);
				bgr_trg = bgr_y(_window);
				meanStdDev(src_trg, src_m, src_std);
				meanStdDev(bgr_trg, bgr_m, bgr_std);

				if(src_std[0] == 0) src_std[0] = 0.00001;
				if(bgr_std[0] == 0) bgr_std[0] = 0.00001;
				covar = (src_trg - src_m).dot(bgr_trg - bgr_m) / _window_area;
				if(covar == 0) covar = 0.00001;
				correl = covar / (src_std[0] * bgr_std[0]);
				rst.at<uint8_t>(y + _window_pad.height,
								x + _window_spad.width) = (correl > 1 ? 1 : correl) * 255;
			}
		}

		rst = rst < (255 / 8);
		erode(rst, rst, _erod_kernel);
		dilate(rst, rst, _dila_kernel);
		rst = rst < 1;

		_last_mask = rst;
		bgr_.copyTo(_last_masked_shot, rst);

		imshow("src", src);
		imshow("bgr", bgr);
		imshow("mask", rst);
		waitKey(33);

		return  rst;
	}
};