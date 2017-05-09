#pragma once
#include <cstdint>
#include <cmath>
#include <opencv2/opencv.hpp>

//
// ver 0.01
//

cv::Mat find_foreground(const cv::Mat& src,
						const cv::Mat& background){
	using namespace cv;
	enum BGR: uint8_t{ B, G, R };
	const auto src_s = src.size();

	Mat diff(Size(src_s.width * 3, src_s.height), CV_8UC1);
	Mat rst(src_s, CV_8UC1);

	for(auto y = 0; y < src_s.height; ++y){
		for(auto x = 0; x < src_s.width; ++x){
			const auto src_pix = src.at<Vec3b>(y, x);
			const auto bgr_pix = background.at<Vec3b>(y, x);
			for(auto i = 0; i < 3; ++i){
				const auto dif = int(src_pix[i]) - int(bgr_pix[i]);
				diff.at<uint8_t>(y, x * 3 + i)
					= std::abs(dif) < 20 ? 255 : 0;
			}
		}
	}
	

	for(auto y = 0; y < src_s.height; ++y){
		for(auto x = 0; x < src_s.width; ++x){
			auto sum = 0;
			for(auto i = 0; i < 3; ++i){
				sum += diff.at<uint8_t>(y, x * 3 + i);
			}
			rst.at<uint8_t>(y, x) = sum / 3;
		}
	}

	{ // open
		erode(rst, rst, Mat());
		dilate(rst, rst, Mat());
	}

	return rst;
}