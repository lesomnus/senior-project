#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <array>


// 0 1
// 2 3
std::array<cv::Point, 4> find_corners(const cv::Mat& src){
	using namespace std;
	using namespace cv;
	enum LOC{ LT, RT, LB, RB };

	constexpr float thresh = 150;
	const Mat& gray = src; //cvtColor(src, gray, COLOR_BGR2GRAY);
	Mat harris;
	{	// harris corner params
		constexpr int		max_thresh = 255;
		constexpr int		blockSize = 3;
		constexpr int		ksize = 3;
		constexpr double	k = 0.04;
		cornerHarris(gray, harris, blockSize, ksize, k);
	}

	Mat dst_norm; normalize(harris, dst_norm, 0, 255, NORM_MINMAX, CV_32FC1);
	Mat dst_norm_scaled; convertScaleAbs(dst_norm, dst_norm_scaled);

	vector<Point> corners;
	{
		const auto size = dst_norm.size();
		for(auto y = 0; y < size.height; ++y){
			for(auto x = 0; x < size.width; ++x){
				if(dst_norm.at<float>(y, x) < thresh) continue;
				corners.push_back(Point(x, y));
			}
		}
	}

	int x_min, x_max, y_min, y_max;
	{
		x_min = x_max = corners[0].x;
		y_min = y_max = corners[0].y;

		for(const auto& point : corners){
			x_min = min(x_min, point.x);
			x_max = max(x_max, point.x);
			y_min = min(y_min, point.y);
			y_max = max(y_max, point.y);
		}
	}

	array<Point, 4> rst;
	{
		array<vector<Point>, 4> set;
		{
			auto x_thrsh = (x_max + x_min) / 2;
			auto y_thrsh = (y_max + y_min) / 2;
			for(const auto& point : corners){
				uint8_t dtor = 0;
				dtor |= point.x > x_thrsh ? RT : 0;
				dtor |= point.y > y_thrsh ? LB : 0;

				set[dtor].push_back(point);
			}
		}

		for(auto i = 0; i < 4; ++i){
			const auto& set_ = set[i];
			auto x_sum = 0;
			auto y_sum = 0;
			for(const auto& point : set_){
				x_sum += point.x;
				y_sum += point.y;
			}
			rst[i] = Point(
				x_sum / set_.size(),
				y_sum / set_.size());
		}
	}

	return rst;
}