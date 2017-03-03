#pragma once
#include <cassert>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <mutex>
#include <thread>

#include <smns/DoubleBuffer.hpp>
#include <smns/Pipe.hpp>

#include "ImProcessor.hpp"

class Coupler{
private:
	using Mtx = std::mutex;
	using Mat = cv::Mat;
	using Dbuf = smns::DoubleBuffer<Mat>;
	using Pipe = smns::Pipe;
	using Thread = std::thread;
public:
	Coupler():_is_open(false), _is_attached(false){ _open(); }
	~Coupler(){ close(); }

	void close(){
		if(!_is_open) return;
		_is_open = false;

		_pipe.close();
		_buff.close();
	}
	void push(Mat&& img){
		if(!_is_attached)
			return _buff.push(std::move(img));

		_buff << img;
		_middle << img;
	}
	Mat pop(){
		//if(!_is_attached)
			return std::move(_buff.pop());
	}
	void attach(){
		if(_is_attached) return;
		_is_attached = true;
	}
	void detach(){
		if(!_is_attached) return;
		_is_attached = false;
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
	ImProcessor _middle;
	Mat _middle_temp;
	Dbuf _buff;
	Pipe _pipe;

	void _open(){
		if(_is_open) return;
		_is_open = true;

		_buff.open();
	}
};