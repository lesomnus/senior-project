#pragma once
#include <cassert>
#include <opencv2/opencv.hpp>
#include <mutex>
#include <thread>
#include <deque>

#include <smns/DoubleBuffer.hpp>

#include "PerspectiveProjector.hpp"
#include "KinectCapture.hpp"

class ImProcessor{
private:
	using Mtx = std::mutex;
	using Mat = cv::Mat;
	using Dbuf = smns::DoubleBuffer<Mat>;
	using Thread = std::thread;
public:
	ImProcessor(int vdevice_index = 0){
		_pre_proc();
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
	KinectColorCapture _color_cap;
	KinectDepthCapture _depth_cap;
	Mat _depth_background;
	PerspectiveProjector _back_projector;

	void _pre_proc(){
		//_back_projector.auto_init(_color_cap);

		constexpr auto sample_s = 4;
		Mat shot; _depth_cap >> shot;
		_depth_background = shot * (1.0 / sample_s);
		for(auto i = 0; i < sample_s - 1; ++i)
			_depth_background += shot * (1.0 / sample_s);
	}

	void _proc_body(){
		using namespace cv;
		while(_in_buff.is_open()){
			Mat img = _in_buff.pop();
			Mat shot; _depth_cap >> shot;
			//imshow("asdf", (shot - _depth_background) > 7);
			//waitKey(1);

			Mat projected_area = _back_projector(shot);
			Mat mask(projected_area.size(), CV_8UC1); mask = Scalar(0xFF);

			mask -= ((projected_area - _depth_background) > 10);
			
			resize(mask, mask, img.size());
			{
				std::lock_guard<Mtx> guard(_out_lock);
				_out_buff = mask;
			}
		}
	}
};