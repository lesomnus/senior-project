#pragma once
#include <cstdint>
#include <cmath>
#include <opencv2/opencv.hpp>

#include <string>
#include <fstream>
#include "correlation.hpp"

void writeCSV(std::string filename, cv::Mat m){
	using namespace std;
	ofstream myfile;
	myfile.open(filename.c_str());
	myfile << cv::format(m, cv::Formatter::FMT_CSV) << std::endl;
	myfile.close();
}

cv::Mat find_foreground(const cv::Mat& src,
						const cv::Mat& background_){
	using namespace cv;
	constexpr bool BGR_BLURRED = true;
	Mat rst;
	Mat background = background_.clone();
	auto src_s = src.size();
	if(BGR_BLURRED){
		GaussianBlur(background, background, Size(5, 5), 1.5);
		imshow("blurred bgr", background);
	}

	constexpr unsigned int WINDOW_W = 16;
	constexpr unsigned int WINDOW_H = 8;
	constexpr unsigned int WINDOW_N = WINDOW_W * WINDOW_H;
	constexpr unsigned int PAD_X = WINDOW_W - 1; // padding of X-axis
	constexpr unsigned int PAD_Y = WINDOW_H - 1; // padding of Y-axis
	constexpr unsigned int SPAD_X = PAD_X / 2;
	constexpr unsigned int SPAD_Y = PAD_Y / 2;
	Mat _src; cvtColor(src, _src, CV_BGR2YCrCb);
	Mat _bgr; cvtColor(background, _bgr, CV_BGR2YCrCb);
	Rect window(0, 0, WINDOW_W, WINDOW_H);
	Mat corrl(src_s, CV_8UC1); corrl = Scalar(255);

	extractChannel(_src, _src, 0);
	extractChannel(_bgr, _bgr, 0);

	Mat src_itg; integral(_src, src_itg, CV_64FC1);
	Mat bgr_itg; integral(_bgr, bgr_itg, CV_64FC1);

	for(auto y = 0; y < src_s.height - PAD_Y; ++y){
		for(auto x = 0; x < src_s.width - PAD_X; ++x){
			window.x = x; window.y = y;
			auto correl = coefficient_in_window(_src, src_itg,
												_bgr, bgr_itg,
												window);
			//Mat src_trg = _src(window);
			//Mat bgr_trg = _bgr(window);
			//Scalar src_m, src_std, bgr_m, bgr_std;
			//meanStdDev(src_trg, src_m, src_std);
			//meanStdDev(bgr_trg, bgr_m, bgr_std);
			//if(src_std[0] == 0) src_std[0] = 0.00001;
			//if(bgr_std[0] == 0) bgr_std[0] = 0.00001;
			//auto covar = (src_trg - src_m).dot(bgr_trg - bgr_m) / WINDOW_N;
			//if(covar == 0) covar = 0.00001;
			//auto correl = covar / (src_std[0] * bgr_std[0]);
			
			corrl.at<uint8_t>(y + PAD_Y, x + SPAD_X) = (correl > 1 ? 1 : correl) * 255;
		}
	}
	rst = corrl < (255 / 8);
	
	auto ekernel = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
	auto dkernel = getStructuringElement(MORPH_ELLIPSE, Size(20, 20));
	erode(rst, rst, ekernel);
	dilate(rst, rst, dkernel);

	//Mat ones(rst.size(), CV_8UC3); ones = Scalar(0xFF, 0xFF, 0xFF);
	//bitwise_and(ones, src, rst, rst.clone());
	Mat rst_;
	src.copyTo(rst_, rst);

	return rst_;
}

#undef METHOD