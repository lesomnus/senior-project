#pragma once
#include <cassert>
#include <opencv2/opencv.hpp>
#include <mutex>
#include <thread>
#include <deque>

#include <smns/DoubleBuffer.hpp>

#include "PerspectiveProjector.hpp"
#include "FaceDetector.hpp"

class ImProcessor{
private:
	using Mtx = std::mutex;
	using Mat = cv::Mat;
	using Dbuf = smns::DoubleBuffer<Mat>;
	using Thread = std::thread;
public:
	ImProcessor(int vdevice_index = 0)
		: _cam(vdevice_index), _miss_count(0){
		_back_projector.auto_init(_cam);
		auto bound_s = _back_projector.bound().size();
		_face_detector.set_min_size_ratio(1.0 / 10);
		_face_detector.set_max_size_ratio(1.0 / 8);

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
	FaceDetector _face_detector;
	std::deque<cv::Rect> _face_detected;
	size_t _miss_count;


	void _proc_body(){
		using namespace cv;
		while(_in_buff.is_open()){
			Mat img = _in_buff.pop();
			Mat shot; _cam >> shot;
			Mat projected_area = _back_projector(shot);
			Mat mask(projected_area.size(), CV_8UC1); mask = Scalar(0xFF);
			Rect trg;

			auto faces = _face_detector(projected_area);
			if(faces.size() == 0){
				if(_miss_count++ == 31){
					_face_detected.clear();
					_miss_count = 0;
				}
			} else for(auto face : faces){
				_miss_count = 0;
				const auto qsize = _face_detected.size();
				if(qsize == 0){
				} else if(qsize > 15){
					_face_detected.pop_front();
				} else{
					const auto diff = face.x - _face_detected.back().x;
					face.x += diff;
				}
				face.height *= 2;
				_face_detected.push_back(face);
			}

			// merge
			for(auto face : _face_detected){
				if(trg == Rect()) trg = face;
				else trg |= face;
			}


			Rect bound(Point(), mask.size());
			trg &= bound;
			mask(trg) = Scalar(0);


			resize(mask, mask, img.size());
			{
				std::lock_guard<Mtx> guard(_out_lock);
				_out_buff = mask;
			}
		}
	}
};