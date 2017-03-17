#pragma once
#include <assert.h>
#include <cmath>
#include <mutex>
#include <condition_variable>
#include <string>
#include <chrono>

#include <opencv2/opencv.hpp>

class Player{
private:
	using Str = std::string;
	using Mat = cv::Mat;
public:
	Player(const char* winname): Player(Str(winname)){}
	Player(const Str& winname): _winname(winname){}
	void push(const Mat& img){
		if(img.empty()) return;
		Str fps = std::to_string(_get_fps());
		using namespace cv;
		putText(img, fps, {0, 28}, FONT_HERSHEY_SIMPLEX,
				1.2, Scalar(0, 0, 255), 2, 4);
		imshow(_winname, img);
		#if  defined(LINUX)
		waitKey(20);
		#endif
	}
	void push(Mat&& img){
		return push(img);
	}
	void operator << (Mat&& img){ push(img); }
	void operator << (Mat& img){ push(img); }
private:
	Str _winname;

	size_t _get_fps(){
		using namespace std::chrono;
		static system_clock::time_point from = system_clock::now();
		static system_clock::time_point to = system_clock::now();

		to = system_clock::now();
		auto diff = duration_cast<std::chrono::milliseconds>(to - from);
		auto result =  1 / static_cast<float>(diff.count()) * 1000;
		from = to;

		return static_cast<size_t>(std::round(result));
	}
};