#pragma once
#include <cstdint>
#include <cmath>
#include <opencv2/opencv.hpp>

#include <string>
#include <fstream>


//	0	Edge feature
//	1	RGB correlation in window
//	2	Y correlation in window
//	3	max(R,G,B)
#define METHOD 2

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
	Mat background = background_;
	auto src_s = src.size();
	if(BGR_BLURRED){
		GaussianBlur(background, background, Size(5, 5), 1.5);
		imshow("blurred bgr", background);
	}

	#if METHOD == 0			// Edge feature
	Mat edg_src; Canny(src, edg_src, 10, 100);
	Mat edg_bgr;
	if(BGR_BLURRED) Canny(background, edg_bgr, 10, 100);
	else Canny(background, edg_bgr, 100, 200);

	imshow("edge of src", edg_src);
	imshow("edge of bgr", edg_bgr);

	dilate(edg_bgr, edg_bgr, getStructuringElement(MORPH_RECT, {3, 3}));
	rst = edg_src - edg_bgr;

	#elif METHOD == 1		// RGB correlation in window
	constexpr unsigned int WINDOW_W = 21;
	constexpr unsigned int WINDOW_H = 21;
	constexpr unsigned int WINDOW_N = WINDOW_W * WINDOW_H;
	constexpr unsigned int PAD_X = WINDOW_W - 1; // padding of X-axis
	constexpr unsigned int PAD_Y = WINDOW_H - 1; // padding of Y-axis
	Mat srcf; src.convertTo(srcf, CV_32FC3);
	Mat bgrf; bgrf.convertTo(bgrf, CV_32FC3);
	Rect window(0, 0, WINDOW_W, WINDOW_H);
	Mat corrl(Size((src_s.width - PAD_X) * 3,
				   src_s.height - PAD_Y), CV_8UC1);
	for(auto i = 0; i < 3; ++i){
		Mat _src; extractChannel(src, _src, i);
		Mat _bgr; extractChannel(background, _bgr, i);
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
				corrl.at<uint8_t>(y, x * 3 + i) = (correl > 1 ? 1 : correl) * 255;
			}
		}
	}
	rst = corrl < (255 / 8);

	#elif METHOD == 2		// Y correlation in window
	constexpr unsigned int WINDOW_W = 21;
	constexpr unsigned int WINDOW_H = 21;
	constexpr unsigned int WINDOW_N = WINDOW_W * WINDOW_H;
	constexpr unsigned int PAD_X = WINDOW_W - 1; // padding of X-axis
	constexpr unsigned int PAD_Y = WINDOW_H - 1; // padding of Y-axis
	Mat src_ycc; cvtColor(src, src_ycc, CV_BGR2YCrCb);
	Mat bgr_ycc; cvtColor(background, bgr_ycc, CV_BGR2YCrCb);
	Rect window(0, 0, WINDOW_W, WINDOW_H);
	Mat corrl(Size((src_s.width - PAD_X - 1),
				   src_s.height - PAD_Y - 1), CV_8UC1);

	Mat _src; extractChannel(src_ycc, _src, 0);
	Mat _bgr; extractChannel(bgr_ycc, _bgr, 0);

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

	#elif METHOD == 3
	rst = Mat(src_s, CV_8UC1);
	constexpr unsigned int WINDOW_W = 21;
	constexpr unsigned int WINDOW_H = 21;
	constexpr unsigned int WINDOW_N = WINDOW_W * WINDOW_H;
	constexpr unsigned int PAD_X = WINDOW_W - 1; // padding of X-axis
	constexpr unsigned int PAD_Y = WINDOW_H - 1; // padding of Y-axis
	Rect window(0, 0, WINDOW_W, WINDOW_H);
	Mat _rst(Size((src_s.width - PAD_X - 1),
				   src_s.height - PAD_Y - 1), CV_8UC1);

	auto max_index = [](const Scalar& src){
		if(src[0] > src[1] && src[0] > src[2]){
			return 0;
		} else if(src[1] > src[0] && src[1] > src[2]) return 1;
		else return 2;
	};

	for(auto y = 0; y < src_s.height - PAD_Y; ++y){
		for(auto x = 0; x < src_s.width - PAD_X; ++x){
			window.x = x; window.y = y;
			Mat src_trg = src(window);
			Mat bgr_trg = background(window);
			auto src_trg_sum = sum(src_trg);
			auto bgr_trg_sum = sum(bgr_trg);
			if(max_index(src_trg_sum) == max_index(bgr_trg_sum)){
				_rst.at<uint8_t>(y, x) = 255;
			} else _rst.at<uint8_t>(y, x) = 0;
		}
	}

	rst = _rst;

	#endif


	return rst;
}

#undef METHOD