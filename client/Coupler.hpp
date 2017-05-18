#pragma once
#include <cassert>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <mutex>
#include <thread>

#include <smns/DoubleBuffer.hpp>
#include <smns/Pipe.hpp>

#include "ImProcessor.hpp"

constexpr int VIDEO_DEVICE_INDEX = 0;

class Coupler{
private:
	using Mtx = std::mutex;
	using Mat = cv::Mat;
	using Dbuf = smns::DoubleBuffer<Mat>;
	using Pipe = smns::Pipe;
	using Thread = std::thread;
public:
	Coupler():
		_is_open(false), _is_attached(false),
		_mid(VIDEO_DEVICE_INDEX){
		_open();
	}
	~Coupler(){ close(); }

	void close(){
		if(!_is_open) return;
		_is_open = false;

		_buff.close();
		_pipe.close();
	}
	void push(Mat&& img){
		if(!_is_attached)
			return _buff.push(std::move(img));

		_buff << img;
		_mid << img;
	}
	Mat pop(){
		if(!_is_attached)
			return std::move(_buff.pop());

		std::lock_guard<Mtx> guard(_mid_temp_lock);
		if(_mid.is_empty()){
			if(_mid_temp.empty()){
				return std::move(_buff.pop());
			} else{
				return _couple(_buff.pop(), _mid_temp);
			}
		} else{
			_mid_temp = _mid.pop();
			return _couple(_buff.pop(), _mid_temp);
		}
	}
	void attach(){
		if(_is_attached) return;
		_is_attached = true;
	}
	void detach(){
		if(!_is_attached) return;
		_is_attached = false;

		{
			std::lock_guard<Mtx> guard(_mid_temp_lock);
			_mid_temp.release();
		}
	}

	template<typename Tistream>
	void pipe(Tistream& istream){
		_pipe.connect(*this, istream);
	}

	void operator << (Mat&& img){ push(std::move(img)); }
	template<typename Tstream>
	void operator >> (Tstream& stream){ stream << std::move(pop()); }
private:
	bool _is_open;
	bool _is_attached;
	ImProcessor _mid;
	Mat _mid_temp;
	Dbuf _buff;
	Pipe _pipe;

	Mtx _mid_temp_lock;

	void _open(){
		if(_is_open) return;
		_is_open = true;

		_buff.open();
	}

	Mat _couple(Mat&& lh, Mat& rh){
		return _couple(lh, rh);
	}
	Mat _couple(Mat& lh, Mat& rh){
		Mat result;
		lh.copyTo(result, rh);

		return result;
	}
};