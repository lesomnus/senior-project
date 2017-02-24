#pragma once
#include <assert.h>
#include <string>
#include <array>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <opencv2/opencv.hpp>
#include <smns/io/sock.hpp>
#include <smns/util/screen.hpp>

#include "DesktopStreamerCmd.h"

using namespace smns;

class DesktopStreamer{
private:
	using Str = std::string;
	using Mtx = std::mutex;
	using Cond = std::condition_variable;
	using Addr = smns::io::Addr;
	using Sock = smns::io::Sock;
	using Buff = smns::io::Buffer;
	using Cmds = std::array<
		std::function<void(void)>,
		DesktopStreamerCmd::_SIZE>;

public:
	DesktopStreamer(Addr address)
		: _is_open(false), _is_client_ready(false),
		_app(io::Domain::INET, io::Type::TCP){
		_app.addr(address).bind();
		assert(!_app.is_error() && "_app.bind() error");

		using namespace DesktopStreamerCmd;
		_cmnds[READY_TO_RCV] = std::bind(&_on_ready, this);
	};
	~DesktopStreamer(){};

	// should be atomic
	void open(){
		if(_is_open) return;
		_is_open = true;
		std::thread(&_receiver, this).detach();
		std::thread(&_acceptor, this).detach();
	}

	// should be atomic
	void close(){
		if(!_is_open) return;
		_is_open = false;
		_app.close();
	}

private:
	bool _is_open;
	bool _is_client_ready;
	Sock _app;
	Cmds _cmnds;

	// wait stream until
	// client ready
	Mtx _stream_lock;
	Cond _until_client_ready;

	static void _log(const char* msg){
		std::clog << msg << std::endl;
	}
	void _acceptor(){
		_app.listen(1);
		_log("Ready to accept.");
		while(_is_open){
			Sock conn = _app.accept();

			// is app closed?
			// then break
			if(conn.is_error()) break;
			else _log("New connection accepted");

			// blocking
			_streamer();

			conn.close();
		}
	}
	void _streamer(){
		{
			std::unique_lock<Mtx> lock(_stream_lock);
			_until_client_ready.wait(lock, [this]{
				return _is_client_ready;
			});
		}
	}
	void _receiver(){
		Buff rcv_buff;
		while(_is_open){
			_app.recv(rcv_buff, 1);
			const uint8_t cmnd = rcv_buff[0];
			if(cmnd > 0) _cmnds[cmnd];
		}
	}

	//
	// commands
	//
	void _on_ready(){
		{
			std::lock_guard<Mtx> lock(_stream_lock);
			_is_client_ready = true;
		}
		_until_client_ready.notify_all();
	}
};