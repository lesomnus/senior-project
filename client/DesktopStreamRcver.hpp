#pragma once
#include <cassert>
#include <vector>
#include <thread>

#include <opencv2/opencv.hpp>
#include <smns/io/sock.hpp>
#include <smns/util/screen.hpp>
#include <smns/DoubleBuffer.hpp>

#include "DesktopStreamerCmd.h"

class DesktopStreamRcver{
private:
	using Mat = cv::Mat;

	using Addr = smns::io::Addr;
	using Sock = smns::io::Sock;
	using Buff = smns::io::Buffer;
	using DBuf = smns::DoubleBuffer<Mat>;

public:
	DesktopStreamRcver(Addr address)
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
		_threads.push_back(
			std::thread(&DesktopStreamRcver::_receiver, this));
		_cmd_ready();
	}
	void close(){
		if(!_is_open) return;
		_is_open = false;

		for(auto& thread : _threads)
			thread.join();
		_threads.clear();
	}
	Mat pop(){
		return std::move(_shot_buff.pop());
	}
private:
	std::vector<std::thread> _threads;
	bool _is_open;
	Sock _conn;
	DBuf _shot_buff;
	uint32_t _chunk_s;

	void _receiver(){
		Buff shot;
		Buff buff;
		Mat encoded;
		Mat decoded;
		_shot_buff.open();
		while(_is_open){
			_conn.recv(buff, 4);
			if(_conn.is_error()){};
			const uint32_t size = *(reinterpret_cast<uint32_t*>(buff.data()));
			const uint32_t thrsh_ready = size * 0.6; // TODO: make user definable. is it need?
			uint32_t remain = size;
			bool is_ready = false;

			while(remain > 0){
				uint32_t chunk_s
					= remain < _chunk_s
					? remain : _chunk_s;

				_conn.recv(buff, chunk_s);
				if(_conn.is_error()){}
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

			encoded = cv::Mat(1, shot.size(), CV_8UC1, shot.data());
			decoded = cv::imdecode(encoded, CV_LOAD_IMAGE_COLOR);
			// TODO: handle decode fail

			shot.clear();
			buff.clear();
			
			_shot_buff.push(std::move(decoded));
		}
	}
	void _send_cmd(DesktopStreamerCmd::cmd cmd){
		static std::vector<uint8_t> buff;
		buff.push_back(static_cast<uint8_t>(cmd));
		_conn.send(buff);
		buff.clear();
	}
	void _cmd_ready(){
		_send_cmd(DesktopStreamerCmd::READY_TO_RCV);
	}
	// TODO: Size command
};