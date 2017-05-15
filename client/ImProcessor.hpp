#pragma once
#include <cassert>
#include <opencv2/opencv.hpp>
#include <mutex>
#include <thread>

#include <smns/DoubleBuffer.hpp>

#include "PerspectiveProjector.hpp"
#include "ForegroundDetector.hpp"

class ImProcessor{
private:
	using Mtx = std::mutex;
	using Mat = cv::Mat;
	using Dbuf = smns::DoubleBuffer<Mat>;
	using Thread = std::thread;
public:
	ImProcessor(int vdevice_index = 0): _cam(vdevice_index){
		_back_projector.auto_init(_cam);
		auto bound_s = _back_projector.bound().size();
		_foreground_detector.set_size(bound_s);

		_in_buff.open();
		_proc = Thread(&ImProcessor::_proc_body, this);
	}
	~ImProcessor(){
		_in_buff.close();
		_proc.join();
	}

	bool is_empty(){
		bool rst = true;
		{
			std::lock_guard<Mtx> guard(_out_lock);
			rst = _out_buff.empty();
		}
		return rst;
	}
	void push(const Mat& img){ _in_buff.push(img); }
	Mat pop(){
		std::lock_guard<Mtx> guard(_out_lock);
		return std::move(_out_buff);
	}

	void operator << (const Mat& img){ push(img); }
private:
	Dbuf _in_buff;
	Mat _out_buff;
	Mtx _out_lock;
	Thread _proc;
	cv::VideoCapture _cam;
	PerspectiveProjector _back_projector;
	ForegroundDetector _foreground_detector;

	void _proc_body(){
		using namespace cv;
		while(_in_buff.is_open()){
			Mat img = _in_buff.pop();
			Mat shot; _cam >> shot;
			Mat projected_area = _back_projector(shot);
			Mat mask = _foreground_detector(projected_area, img);

			resize(mask, mask, img.size());

			{
				std::lock_guard<Mtx> guard(_out_lock);
				_out_buff = mask;
			}
		}
	}
};