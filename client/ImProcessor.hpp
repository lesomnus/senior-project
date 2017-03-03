#pragma once
#include <assert.h>
#include <opencv2/opencv.hpp>
#include <mutex>
#include <thread>

#include <smns/DoubleBuffer.hpp>

class ImProcessor{
private:
	using Mtx = std::mutex;
	using Mat = cv::Mat;
	using Dbuf = smns::DoubleBuffer<Mat>;
	using Thread = std::thread;
public:
	ImProcessor(){
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
	void push(Mat& img){ _in_buff.push(img); }
	Mat pop(){
		std::lock_guard<Mtx> guard(_out_lock);
		return std::move(_out_buff);
	}
private:
	Dbuf _in_buff;
	Mat _out_buff;
	Mtx _out_lock;
	Thread _proc;

	void _proc_body(){
		while(_in_buff.is_open()){
			Mat img = _in_buff.pop();
			Mat result(img.size(), img.type()); // TODO: result will binary

			//
			// body
			//


			{
				std::lock_guard<Mtx> guard(_out_lock);
				_out_buff = result;
			}
		}
	}
};