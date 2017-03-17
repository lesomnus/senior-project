#pragma once
#include <cassert>
#include <vector>
#include <thread>
#include <string>

#include <opencv2/opencv.hpp>
#include <smns/io/Sock.hpp>
#include <smns/DoubleBuffer.hpp>
#include <smns/Pipe.hpp>

#include "DesktopStreamerCmd.h"

class DesktopStreamRcver{
private:
	using Mat = cv::Mat;
	using Size = cv::Size;

	using Addr = smns::io::Addr;
	using Sock = smns::io::Sock;
	using Buff = smns::io::Buffer;
	using DBuf = smns::DoubleBuffer<Mat>;
	using Pipe = smns::Pipe;

public:
	DesktopStreamRcver(Addr& address)
		:_is_open(false), _chunk_s(256),
		_conn(smns::io::Domain::INET,
			  smns::io::Type::TCP){
		_conn.addr(address);
	}
	~DesktopStreamRcver(){ close(); }

	bool is_open(){ return _is_open; }
	bool is_close(){ return !is_open(); }
	void open(){
		if(_is_open) return;
		_is_open = true;
		_conn.connect();
		_cmd_meta_size();
		_threads.push_back(
			std::thread(&DesktopStreamRcver::_receiver, this));
		_cmd_ready();
	}
	void close(){
		if(!_is_open) return;
		_is_open = false;

		_shot_buff.close();
		_pipe.close();
		for(auto& thread : _threads)
			thread.join();
		_threads.clear();
	}
	Mat pop(){
		return std::move(_shot_buff.pop());
	}

	template<typename Tistream>
	void pipe(Tistream& istream){
		_pipe.connect(_shot_buff, istream);
	}
private:
	std::vector<std::thread> _threads;
	bool _is_open;
	Sock _conn;
	DBuf _shot_buff;
	uint32_t _chunk_s;
	Pipe _pipe;

	void _receiver(){
		static Mat disp_lost;
		Buff shot;
		Buff buff;
		Mat encoded;
		Mat decoded;
		Size last_s = {1280, 800};

		// connection params
		size_t retry = 0;
		bool is_lost = false;

		_shot_buff.open();
		while(_is_open){
			//{ // for 10 fps test
			//	using namespace std::chrono_literals;
			//	std::this_thread::sleep_for(100ms);
			//}
			_conn.recv(buff, 4);

			// handle connection error
			while((is_lost || _conn.is_error()) && _is_open){
				{	// reconnect proc
					using namespace smns::io;
					_conn = Sock(Domain::INET, Type::TCP)
						.addr(_conn.addr());
					_conn.connect();

					// if connection successful
					if(_conn.val() == 0){
						_cmd_meta_size();
						_cmd_ready();
						_conn.recv(buff, 4);
						if(_conn.is_error())
							// connected but
							// Immediately disconnected
							continue;

						// OK. It's fine to continue
						retry = 0;
						is_lost = false;
						break;
					}
				}

				{	// make inform display proc
					using namespace cv;
					std::string msg = "Connection lost(";
					msg = msg + std::to_string(++retry) + ")";

					if(disp_lost.empty() || disp_lost.size() != last_s){
						disp_lost = Mat(last_s, CV_8UC3);
					}

					disp_lost.setTo(Scalar(0, 0, 0));
					putText(disp_lost, msg, {0, 60}, FONT_HERSHEY_SIMPLEX,
							1.2, Scalar(255, 255, 255), 2, 4);
				}

				_shot_buff.push(disp_lost);

				{	// wait for timeout
					using namespace std::chrono_literals;
					std::this_thread::sleep_for(500ms);
				}
			}
			if(!_is_open) break;

			const uint32_t size = *(reinterpret_cast<uint32_t*>(buff.data()));
			const uint32_t thrsh_ready = size * 0.1; // TODO: make user definable. is it need?
			uint32_t remain = size;
			bool is_ready = false;

			while(remain > 0){
				uint32_t chunk_s
					= remain < _chunk_s
					? remain : _chunk_s;

				_conn.recv(buff, chunk_s);
				if(_conn.is_error()){
					is_lost = true;
					break;
				}
				if(_conn.val() != chunk_s){
					chunk_s = _conn.val();
				}
				remain -= chunk_s;
				if(!is_ready && remain < thrsh_ready){
					_cmd_ready();
					is_ready = true;
				}
				shot.insert(shot.end(), buff.begin(), buff.end());
			}
			if(is_lost){
				shot.clear();
				buff.clear();
				continue;
			}

			encoded = cv::Mat(1, shot.size(), CV_8UC1, shot.data());
			decoded = cv::imdecode(encoded, CV_LOAD_IMAGE_COLOR);
			// TODO: handle decode fail

			shot.clear();
			buff.clear();

			last_s = decoded.size();
			_shot_buff.push(std::move(decoded));
		}
	}
	void _cmd_(DesktopStreamerCmd::cmd cmd,
			   const Buff& data = Buff()){
		static Buff buff;
		buff.push_back(static_cast<uint8_t>(cmd));

		if(!data.empty())
			buff.insert(buff.end(), data.cbegin(), data.cend());

		_conn.send(buff);
		buff.clear();
	}
	void _cmd_ready(){ _cmd_(DesktopStreamerCmd::READY_TO_RCV); }
	void _cmd_meta_size(){
		Buff data;

		#if (defined(_WIN32) || defined (_WIN64) || defined(WIN32) || defined(WIN64))
		// window test env
		auto w = 1280;
		auto h = 800;
		#else
		// <!-- TODO: get screen resolution on linux -->
		// Fix the size
		auto w = 640;
		auto h = 400;
		#endif
		uint32_t temp = ((0xFFFF & w) << 16) | (0xFFFF & h);

		for(auto i = 0; i < 4; ++i){
			data.push_back(*(reinterpret_cast<uint8_t*>(&temp) + i));
		}

		_cmd_(DesktopStreamerCmd::META_SIZE, data);
	}
};