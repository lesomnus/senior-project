#pragma once

#include <opencv2/opencv.hpp>
#include <array>

std::array<cv::Point, 4> scale_from_center(const std::array<cv::Point, 4>& src, double scale){
	using namespace std;
	using namespace cv;

	Point center;
	{
		for(const auto& point : src){
			center.x += point.x;
			center.y += point.y;
		}
		center.x = static_cast<int>(center.x / 4);
		center.y = static_cast<int>(center.y / 4);
	}

	auto rst = src;
	{
		auto f = [scale](auto dst, auto ctr){ return (dst - ctr) * scale + ctr; };
		for(auto& point : rst){
			point = Point(
				f(point.x, center.x),
				f(point.y, center.y));
		}
	}

	return rst;
}