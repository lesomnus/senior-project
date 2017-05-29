/**
* File Name:
*	client/ImProcessor.hpp
* Description:
*	An image processor which uses camera and other moudels.
*
* Programmed by Hwang Seung Huyn
* Check the version control of this file
* here-> https://github.com/lesomnus/senior-project/commits/master/client/Coupler.hpp
*/

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
		_erode_kernerl = getStructuringElement(cv::MorphShapes::MORPH_RECT, cv::Size(10, 10));

		_pre_proc();
		_in_buff.open();
		_proc = Thread(&ImProcessor::_proc_body, this);
	}
	~ImProcessor(){
		_in_buff.close();
		_proc.join();
	}
	/**
	*  Function Name: is_empty
	*  Input arguments (condition):
	*	None.
	*  Processing in function (in pseudo code style):
	*	Simple getter.
	*  Function Return: 
	*	None.
	*/
	bool is_empty(){
		bool rst = true;
		{
			std::lock_guard<Mtx> guard(_out_lock);
			rst = _out_buff.empty();
		}
		return rst;
	}

	/**
	*  Function Name: push
	*  Input arguments (condition):
	*	Data to be processed.
	*  Processing in function (in pseudo code style):
	*	1) Push the data.
	*  Function Return: 
	*	None.
	*/
	void push(const Mat& img){ _in_buff.push(img); }

	/**
	*  Function Name: pop
	*  Input arguments (condition):
	*	None.
	*  Processing in function (in pseudo code style):
	*	1) Pop the data from the inner-buffer then return it.
	*  Function Return: 
	*	Processed data.
	*/
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
	Mat _erode_kernerl;
	PerspectiveProjector _back_projector;

	void _pre_proc(){
		_back_projector.auto_init(_color_cap);
		
		constexpr auto sample_s = 4;
		Mat shot; _depth_cap >> shot;
		_depth_background = shot * (1.0 / sample_s);
		for(auto i = 0; i < sample_s - 1; ++i)
			_depth_background += shot * (1.0 / sample_s);

		_depth_background = _back_projector(_depth_background);
	}

	void _proc_body(){
		using namespace cv;
		while(_in_buff.is_open()){
			Mat img = _in_buff.pop();
			Mat shot; _depth_cap >> shot;

			Mat projected_area = _back_projector(shot);
			Mat mask(projected_area.size(), CV_8UC1); mask = Scalar(0xFF);

			mask -= ((projected_area - _depth_background) > 10);


			erode(mask, mask, _erode_kernerl);

			resize(mask, mask, img.size());
			{
				std::lock_guard<Mtx> guard(_out_lock);
				_out_buff = mask;
			}
		}
	}
};