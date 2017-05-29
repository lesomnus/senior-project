/**
* File Name:
*	client/Coupler.hpp
* Description:
*	A coupler that combines two inputs selectively.
*	However, one of input is built-in image processor which can be seen "client/ImProcessor.hpp"
*	So, it combines user-data and built-in image processor output.
*
* Programmed by Hwang Seung Huyn
* Check the version control of this file
* here-> https://github.com/lesomnus/senior-project/commits/master/client/Coupler.hpp
*/

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

	/**
	*  Function Name: close
	*  Input arguments (condition):
	*	None.
	*  Processing in function (in pseudo code style):
	*	1) Close it.
	*  Function Return: 
	*	None.
	*/
	void close(){
		if(!_is_open) return;
		_is_open = false;

		_buff.close();
		_pipe.close();
	}

	/**
	*  Function Name: push
	*  Input arguments (condition):
	*	Data to be combined.
	*  Processing in function (in pseudo code style):
	*	1) Push the data.
	*  Function Return: 
	*	None.
	*/
	void push(Mat&& img){
		if(!_is_attached)
			return _buff.push(std::move(img));

		_buff << img;
		_mid << img;
	}

	/**
	*  Function Name: pop
	*  Input arguments (condition):
	*	None.
	*  Processing in function (in pseudo code style):
	*	1) If "attached", combine two inputs then return it.
	*	2) If not, just return input data.
	*  Function Return: 
	*	Combined data.
	*/
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

	/**
	*  Function Name: attach
	*  Input arguments (condition):
	*	None.
	*  Processing in function (in pseudo code style):
	*	1) Attach built-in image processor.
	*  Function Return: 
	*	None.
	*/
	void attach(){
		if(_is_attached) return;
		_is_attached = true;
	}

	/**
	*  Function Name: dettach
	*  Input arguments (condition):
	*	None.
	*  Processing in function (in pseudo code style):
	*	1) Dettach built-in image processor.
	*  Function Return: 
	*	None.
	*/
	void detach(){
		if(!_is_attached) return;
		_is_attached = false;

		{
			std::lock_guard<Mtx> guard(_mid_temp_lock);
			_mid_temp.release();
		}
	}

	/**
	*  Function Name: pipe
	*  Input arguments (condition):
	*	An input stream that receives data.
	*  Processing in function (in pseudo code style):
	*	1) Pop the data from the inner-buffer then return it.
	*  Function Return: 
	*	Connect between inner-buffer and input.
	*/
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