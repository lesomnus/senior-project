#if defined(_DEBUG)
#	define IFNDBG(expr) {}
#else
#	define IFNDBG(expr) expr
#endif

#include <iostream>
#include <cstdint>
#include <string>
#include <opencv2/opencv.hpp>

#include "find_projected_area.hpp"
#include "find_corners.hpp"
#include "scale_from_center.hpp"
#include "PerspectiveProjector.hpp"
#include "find_foreground.hpp"

int main(){
	constexpr size_t DFT_SRC_BRIGHTNESS_LEVEL = 0;
	constexpr char* RESOURCE_PATH = "./resource/";
	using namespace std;
	using namespace cv;
	size_t src_brightness_level = 0;
	
	auto get_resource_name = [&src_brightness_level, RESOURCE_PATH](const char* type){
		string name;
		name += RESOURCE_PATH;
		name += type;
		name += to_string(src_brightness_level);
		name += ".png";
		return name;
	};

	Mat src;
	{
		cout << "Choose brightness level to test between 1 and 3, other to default. \n"
			<< "The smaller the number, the darker it is." << endl;

		cin >> src_brightness_level;
		cin.clear();

		if(src_brightness_level > 3 || src_brightness_level == 0)
			src_brightness_level = DFT_SRC_BRIGHTNESS_LEVEL;

		string filename = get_resource_name("pattern");
		src = imread(filename);
		if(!src.data){
			cout << "Unable to load. -> " << filename << endl;
			return -1;
		}
	}

	//
	// source pattern image
	//
	imshow("src", src);
	IFNDBG(waitKey(0));

	//
	// area found
	//
	Mat projected_area = find_projected_area(src);
	imshow("projected area", projected_area);
	IFNDBG(waitKey(0));

	//
	// false corner found
	//
	using Corners = std::array<Point, 4>;
	Mat corner_found = src.clone();
	Corners false_corners = find_corners(projected_area);
	for(const auto& corner : false_corners){
		circle(corner_found, corner, 3, Scalar(0, 0, 255), -1);
	}
	imshow("corner found", corner_found);
	IFNDBG(waitKey(0));

	//
	// true coner found
	// using perspective projection
	//
	using Projector = PerspectiveProjector<cv::Vec3b>;
	Projector back_projector(false_corners);
	Corners true_corners;
	{
		auto bound = back_projector.bound();
		true_corners = scale_from_center({
			Point(0,0), Point(bound.width, 0), Point(0, bound.height),
			Point(bound.width, bound.height)
		}, 2);
		for(auto& corner : true_corners){
			corner = back_projector(corner);
		}
	}
	for(const auto& corner : true_corners){
		circle(corner_found, corner, 3, Scalar(255, 0, 0), -1);
	}
	imshow("corner found", corner_found);
	IFNDBG(waitKey(0));

	//
	// source shot
	//
	Mat src_shot = imread(get_resource_name("shot"));
	imshow("shot", src_shot);
	IFNDBG(waitKey(0));

	//
	// project source shot
	//
	back_projector.eval(true_corners);
	Mat projected = back_projector(src_shot);
	imshow("projected shot", projected);
	IFNDBG(waitKey(0));

	//
	// source fore shot
	//
	Mat src_fore_shot = imread(get_resource_name("fore"));
	imshow("fore shot", src_fore_shot);
	IFNDBG(waitKey(0));

	//
	// project source fore shot
	//
	Mat fore_projected = back_projector(src_fore_shot);
	imshow("projected fore shot", fore_projected);
	IFNDBG(waitKey(0));

	//
	// destroy not necessary windows
	//
	destroyWindow("src");
	destroyWindow("projected area");
	destroyWindow("shot");
	destroyWindow("projected shot");

	//
	// original shot
	//
	Mat org_shot = imread(get_resource_name("org"));
	resize(org_shot, org_shot, org_shot.size() / 4);
	imshow("original shot", org_shot);
	IFNDBG(waitKey(0));

	//
	// resize projected fore shot to org's size
	//
	resize(fore_projected, fore_projected, org_shot.size());
	imshow("projected fore shot", fore_projected);

	//
	// find foreground
	//
	Mat foreground = find_foreground(fore_projected, org_shot);
	//normalize(foreground, foreground, 0, 255, cv::NORM_MINMAX);
	imshow("foreground", foreground);

	waitKey(0);
	return 0;
}