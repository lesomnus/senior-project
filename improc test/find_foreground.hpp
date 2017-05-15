#pragma once
#include <cstdint>
#include <cmath>
#include <opencv2/opencv.hpp>

#include <string>
#include <fstream>

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

	constexpr unsigned int WINDOW_W = 21;
	constexpr unsigned int WINDOW_H = 21;
	constexpr unsigned int WINDOW_N = WINDOW_W * WINDOW_H;
	constexpr unsigned int PAD_X = WINDOW_W - 1; // padding of X-axis
	constexpr unsigned int PAD_Y = WINDOW_H - 1; // padding of Y-axis
	Mat _src; cvtColor(src, _src, CV_BGR2YCrCb);
	Mat _bgr; cvtColor(background, _bgr, CV_BGR2YCrCb);
	Rect window(0, 0, WINDOW_W, WINDOW_H);
	Mat corrl(Size((src_s.width - PAD_X - 1),
				   src_s.height - PAD_Y - 1), CV_8UC1);

	extractChannel(_src, _src, 0);
	extractChannel(_bgr, _bgr, 0);

	for(auto y = 0; y < src_s.height - PAD_Y; ++y){
		for(auto x = 0; x < src_s.width - PAD_X; ++x){
			window.x = x; window.y = y;
			Mat src_trg = _src(window);
			Mat bgr_trg = _bgr(window);
			Scalar src_m, src_std, bgr_m, bgr_std;
			meanStdDev(src_trg, src_m, src_std);
			meanStdDev(bgr_trg, bgr_m, bgr_std);
			if(src_std[0] == 0) src_std[0] = 0.00001;
			if(bgr_std[0] == 0) bgr_std[0] = 0.00001;
			auto covar = (src_trg - src_m).dot(bgr_trg - bgr_m) / WINDOW_N;
			if(covar == 0) covar = 0.00001;
			auto correl = covar / (src_std[0] * bgr_std[0]);
			//std::cout << covar << std::endl;
			corrl.at<uint8_t>(y, x) = (correl > 1 ? 1 : correl) * 255;
		}
	}
	rst = corrl < (255 / 6);
	
	auto ekernel = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
	auto dkernel = getStructuringElement(MORPH_ELLIPSE, Size(25, 25));
	erode(rst, rst, ekernel);
	dilate(rst, rst, dkernel);

	return rst;
}

#undef METHOD