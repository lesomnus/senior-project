#if defined(_DEBUG)
#	define IFNDBG(expr) {}
#else
#	define IFNDBG(expr) {} //expr
#endif

#include <iostream>
#include <cstdint>
#include <string>
#include <opencv2/opencv.hpp>
#include <smns/util/measure.hpp>

#include "find_projected_area.hpp"
#include "find_corners.hpp"
#include "scale_from_center.hpp"
#include "PerspectiveProjector.hpp"
#include "find_foreground.hpp"

int main(){
	constexpr char* RESOURCE_PATH = "./resource/";
	constexpr bool IS_WHITE = false;
	using namespace std;
	using namespace cv;

	auto get_resource_name = [RESOURCE_PATH](const char* filename){
		string name;
		name += RESOURCE_PATH;
		name += filename;
		name += ".png";
		return name;
	};

	//
	// source pattern image
	//
	Mat src_pattern = imread(get_resource_name("pattern"));
	imshow("src pattern", src_pattern);
	IFNDBG(waitKey(0));

	//
	// area found
	//
	Mat projected_area = find_projected_area(src_pattern);
	imshow("projected area", projected_area);
	IFNDBG(waitKey(0));

	//
	// corners found
	//
	using Projector = PerspectiveProjector<Vec3b>;
	using Corners = std::array<Point, 4>;
	Projector back_projector;
	Corners corners;
	Size bound_s;
	{
		Corners false_corners = find_corners(projected_area);
		for(const auto& corner : false_corners){
			circle(src_pattern, corner, 3, Scalar(0, 0, 255), -1);
		}
		back_projector = Projector(false_corners);
		auto bound = back_projector.bound();
		bound_s = bound.size();
		corners = scale_from_center({
					Point(0,0), Point(bound.width, 0), Point(0, bound.height),
					Point(bound.width, bound.height)
		}, 4.0 / 3 - 0.0125); // 4 / 3;
		for(auto& corner : corners){
			corner = back_projector(corner);
			circle(src_pattern, corner, 3, Scalar(255, 0, 0), -1);
		}
		back_projector = Projector(corners);
	}
	imshow("src pattern", src_pattern);
	IFNDBG(waitKey(0));
	
	//
	// original (background)
	//
	Mat org = imread(get_resource_name(IS_WHITE ? "orgwhite" : "org"));
	resize(org, org, bound_s);
	imshow("original", org);
	IFNDBG(waitKey(0));

	//
	// shot
	//
	Mat shot = imread(get_resource_name(IS_WHITE ? "forewhite" : "shot_fore"));
	shot = back_projector(shot);
	resize(shot, shot, bound_s);
	imshow("shot", shot);
	IFNDBG(waitKey(0));

	//
	// find foreground
	//
	Mat foreground;

	std::cout << "start" << std::endl;
	for(auto i = 0; i < 1; i++)
		smns::util::omeasure(std::string("test") + std::to_string(i), [&foreground, &shot, &org](){
		foreground = find_foreground(shot, org);
	});
	std::cout << "done" << std::endl;
	 
	//normalize(foreground, foreground, 0, 255, cv::NORM_MINMAX);
	imshow("foreground", foreground);

	waitKey(0);
	return 0;
}