#pragma once

#include <opencv2/opencv.hpp>
#include <cstdint>

//auto coeff_in_window = [](const Mat& src,
//						  const Mat& itg,
//						  const Rect& window,
//						  double& coeff){
//	Mat src_trg = src(window);
//	Mat itg_trg = itg(window);
//	const auto window_area = window.area();
//	const auto sqrt_window_area = cv::sqrt(window_area);
//	double sum = itg_trg.at<double>(window.y + window.height,
//									window.x + window.width)
//		- itg_trg.at<double>(window.y + window.height, window.x)
//		- itg_trg.at<double>(window.y, window.x + window.width)
//		+ itg_trg.at<double>(window.y, window.x);
//	double mean = sum / window_area;
//	double stdev = 0;
//	const double* p = nullptr;
//
//	for(auto y = 0; y < src_trg.rows; ++y){
//		p = src.ptr<double>(y);
//		for(auto x = 0; x < src_trg.cols; ++x)
//			stdev_ += cv::pow(*p - mean, 2);
//	}
//	stdev_ = cv::sqrt(stdev_) / sqrt_window_area;
//
//	mean = mean_;
//	stdDev = stdev_;
//
//	return;
//};
//
//for(auto y = 0; y < src_s.height - PAD_Y; ++y){
//	for(auto x = 0; x < src_s.width - PAD_X; ++x){
//		window.x = x; window.y = y;
//	}
//}
//

//for(auto y = 0; y < src_s.height - PAD_Y; ++y){
//	for(auto x = 0; x < src_s.width - PAD_X; ++x){
//		window.x = x; window.y = y;
//		Mat src_trg = _src(window);
//		Mat bgr_trg = _bgr(window);
//		Scalar src_m, src_std, bgr_m, bgr_std;
//		meanStdDev(src_trg, src_m, src_std);
//		meanStdDev(bgr_trg, bgr_m, bgr_std);
//		if(src_std[0] == 0) src_std[0] = 0.00001;
//		if(bgr_std[0] == 0) bgr_std[0] = 0.00001;
//		auto covar = (src_trg - src_m).dot(bgr_trg - bgr_m) / WINDOW_N;
//		if(covar == 0) covar = 0.00001;
//		auto correl = covar / (src_std[0] * bgr_std[0]);
//		//std::cout << covar << std::endl;
//		corrl.at<uint8_t>(y + PAD_Y, x + SPAD_X) = (correl > 1 ? 1 : correl) * 255;
//	}
//}

double coefficient_in_window(const cv::Mat& src1,
							 const cv::Mat& src1_itg,
							 const cv::Mat& src2,
							 const cv::Mat& src2_itg,
							 const cv::Rect& window){
	using namespace cv;
	static auto calc_sum = [](const Mat& integral_map,
					   const Rect& window){
		return
			integral_map.at<double>(window.y, window.x)
			- integral_map.at<double>(window.y + window.height, window.x)
			- integral_map.at<double>(window.y, window.x + window.width)
			+ integral_map.at<double>(window.y + window.height,
									  window.x + window.width);

	};
	const auto N = window.area();
	const auto sqrt_N = cv::sqrt(N);
	const auto src1_mean = calc_sum(src1_itg, window) / N;
	const auto src2_mean = calc_sum(src2_itg, window) / N;
	Mat src1_roi = src1(window);
	Mat src2_roi = src2(window);

	double src1_stdev = 0;
	double src2_stdev = 0;

	const uint8_t* p1 = nullptr;
	const uint8_t* p2 = nullptr;
	double diff1;
	double diff2;

	for(auto y = 0; y < window.height; ++y){
		p1 = src1_roi.ptr<uint8_t>(y);
		p2 = src2_roi.ptr<uint8_t>(y);
		for(auto x = 0; x < window.width; ++x){
			diff1 = p1[x] - src1_mean;
			diff2 = p2[x] - src2_mean;
			src1_stdev += pow(diff1, 2);
			src2_stdev += pow(diff2, 2);
		}
	}

	src1_stdev = sqrt(src1_stdev) / sqrt_N;
	src2_stdev = sqrt(src2_stdev) / sqrt_N;

	auto cov = (src1_roi - src1_mean).dot(src2_roi - src2_mean) / N;

	if(src1_stdev == 0) src1_stdev = 0.00001;
	if(src2_stdev == 0) src2_stdev = 0.00001;
	if(cov == 0) cov = 0.00001;

	return cov / (src1_stdev * src2_stdev);
}