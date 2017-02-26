#pragma once
#include <cassert>

#include <opencv2/opencv.hpp>
#include <smns/io/sock.hpp>
#include <smns/util/screen.hpp>
#include <smns/DoubleBuffer.hpp>

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
	~DesktopStreamRcver(){}

	bool is_open(){ return _is_open; }
	bool is_close(){ return !is_open(); }
	void open(){
		if(_is_open) return;
		_is_open = true;
	}
	void close(){
		if(!_is_open) return;
		_is_open = false;
	}
private:
	bool _is_open;
	Sock _conn;
	DBuf _shot_buff;
	uint32_t _chunk_s;

	void _receiver(){
		Buff shot;
		Buff buff;
		Mat encoded;
		Mat decoded;
		while(_is_open){
			_conn.recv(buff, 4);
			if(_conn.is_error()){};
			const uint32_t size = *(reinterpret_cast<uint32_t*>(buff.data()));
			const uint32_t thrsh_ready = size * 0.6; // TODO: make user definable.
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
					// TODO: send: ready to receive
					is_ready = true;
				}
				shot.insert(shot.end(), buff.begin(), buff.end());
			}

			shot.clear();
			buff.clear();

			encoded = cv::Mat(1, shot.size(), CV_8UC1, shot.data());
			decoded = cv::imdecode(encoded, CV_LOAD_IMAGE_COLOR);
			
			// TODO: handle decode fail
		}
	}
	// TODO: command sender
};