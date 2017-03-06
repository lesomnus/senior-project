#pragma once
#include <cassert>
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

	void _proc_body(){
		while(_in_buff.is_open()){
			Mat img = _in_buff.pop();
			if(img.empty()) continue;			Mat mask = Mat::zeros(img.size(), CV_8UC1);
			cv::ellipse(mask, cv::Point(mask.cols / 2, mask.rows / 2),
						cv::Size(mask.cols / 2, mask.rows / 2), 0, 0, 360,
						cv::Scalar(255), CV_FILLED, 8, 0);


			{
				std::lock_guard<Mtx> guard(_out_lock);
				_out_buff = mask;
			}
//			Mat result(img.size(), CV_8U); // TODO: result will binary
		}
	}
};