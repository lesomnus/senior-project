#pragma once
#include <assert.h>
#include <mutex>
#include <condition_variable>
#include <string>

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
		using namespace cv;
		imshow(_winname, img);
	}
	void operator << (Mat& img){ push(img); }
private:
	Str _winname;
};