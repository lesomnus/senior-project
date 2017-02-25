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
#include <smns/DoubleBuffer.hpp>

#include "DesktopStreamerCmd.h"

using namespace smns;

class DesktopStreamer{
private:
	using Str = std::string;
	using Mtx = std::mutex;
	using Cond = std::condition_variable;

	using Mat = cv::Mat;

	using Addr = smns::io::Addr;
	using Sock = smns::io::Sock;
	using Buff = smns::io::Buffer;
	using DBuf = smns::DoubleBuffer<Buff>;
	using Cmds = std::array<
		std::function<void(void)>,
		DesktopStreamerCmd::_SIZE>;

public:
	DesktopStreamer(Addr address)
		: _is_open(false), _is_client_ready(false),
		_app(io::Domain::INET, io::Type::TCP){
		_app.addr(address);

		using namespace DesktopStreamerCmd;
		_cmnds[READY_TO_RCV] = std::bind(&DesktopStreamer::_on_ready, this);
	};
	~DesktopStreamer(){
		_app.close();
	};

	// should be atomic
	void open(){
		static bool is_reopen = false;
		if(_is_open) return;
		_is_open = true;
		if(is_reopen)
			_app = Sock(io::Domain::INET,
						io::Type::TCP).addr(_app.addr());
		else is_reopen = true;
		_app.bind();
		std::cout << WSAGetLastError() << std::endl;
		assert(!_app.is_error() && "_app.bind() error");
		_app.listen(1);
		std::thread(&DesktopStreamer::_capturer, this).detach();
		std::thread(&DesktopStreamer::_acceptor, this).detach();
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
	DBuf _out_buff;

	Mtx _log_lock;
	// wait stream until
	// client ready
	Mtx _stream_lock;
	Cond _until_client_ready;

	void _log(const char* msg){
		std::lock_guard<Mtx> lock(_log_lock);
		std::clog << msg << std::endl;
	}
	void _acceptor(){
		_log("Ready to accept.");
		while(_is_open){
			Sock conn = _app.accept();

			// is app closed?
			// then break
			if(conn.val() < 0) break;
			else _log("New connection accepted");

			std::thread(&DesktopStreamer::_receiver,
						this, conn).detach();
			// blocking
			_stream_to(conn);

			conn.close();
		}
		_log("Acceptor down");
	}
	void _stream_to(Sock& conn){
		Buff temp;
		Buff size(4, 0);
		while(_is_open){
			{
				std::unique_lock<Mtx> lock(_stream_lock);
				_until_client_ready.wait(lock, [this]{
					return _is_client_ready;
				});
				_is_client_ready = false;
			}
			temp = std::move(_out_buff.pop());
			if(_out_buff.is_close()) break;

			*(reinterpret_cast<uint32_t*>(size.data()))
				= temp.size();

			if(conn.send(size).is_error()) break;
			if(conn.send(temp).is_error()) break;
		}
	}
	void _receiver(Sock& conn){
		Buff rcv_buff;
		while(_is_open){
			conn.recv(rcv_buff, 1);
			const uint8_t cmnd = rcv_buff[0];
			if(!(cmnd > 0)) break;
			_cmnds[cmnd]();
		}
		_log("Disconnected");
	}
	void _capturer(){
		Mat desktop;
		Buff encoded;
		_out_buff.open();
		while(_is_open){
			smns::util::screen::capture(desktop);
			cv::imencode(".png", desktop, encoded);
			_out_buff.push(std::move(encoded));
		}
		_out_buff.close();
		_log("Capturer down");
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