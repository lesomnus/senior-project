#pragma once

#include <opencv2/opencv.hpp>
#include <queue>
#include <array>

cv::Mat find_projected_area(const cv::Mat& src){
	using namespace cv;

	auto contains = [](const Mat& src, const Point& trg){
		return trg.x > 0 || trg.y > 0
			|| trg.x < src.cols
			|| trg.y < src.rows;
	};

	enum FLAG: uint8_t{ IDLE = 0, NOT = 128, TRG = 255 };
	enum BGR: uint8_t{ B, G, R };

	const auto src_s = src.size();
	Mat rst(src_s, CV_8UC1); rst = Scalar(0);
	std::queue<Point> path;
	std::array<Point, 4> trgs;

	// initial path
	path.push(Point(src_s.width / 2, src_s.height / 2));

	// BFS
	while(!path.empty()){
		const auto cur = path.front();
		trgs = {
			Point(cur.x, cur.y - 1),
			Point(cur.x + 1, cur.y),
			Point(cur.x, cur.y + 1),
			Point(cur.x - 1, cur.y)
		};

		// traversal
		for(const auto& trg : trgs){
			if(!contains(src, trg)				// out of range
			   || rst.at<uint8_t>(trg) != IDLE	// already visited
			   ) continue;

			// visit
			const auto pix = src.at<Vec3b>(trg);
			if(pix[G] > pix[R]){
				rst.at<uint8_t>(trg) = TRG;
				path.push(trg);
			} else rst.at<uint8_t>(trg) = NOT;
		}

		path.pop();
	}

	return rst;
}
