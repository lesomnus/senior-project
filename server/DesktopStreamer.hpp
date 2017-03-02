#pragma once
#include <assert.h>
#include <string>
#include <array>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <opencv2/opencv.hpp>
#include <smns/io/sock.hpp>
#include <smns/util/screen.hpp>
#include <smns/DoubleBuffer.hpp>

#include "DesktopStreamerCmd.h"

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
		std::function<void(Sock&)>,
		DesktopStreamerCmd::_SIZE>;

public:
	DesktopStreamer(Addr address)
		: _is_open(false), _is_client_ready(false),
		_is_connected(false),
		_app(smns::io::Domain::INET,
			 smns::io::Type::TCP){
		_app.addr(address);

		{
			using namespace std::placeholders;
			// set commands
			_cmnds[DesktopStreamerCmd::DISCONNECT]
				= std::bind(&DesktopStreamer::_on_disconnect, this, _1);
			_cmnds[DesktopStreamerCmd::READY_TO_RCV]
				= std::bind(&DesktopStreamer::_on_ready, this, _1);
			_cmnds[DesktopStreamerCmd::META_SIZE]
				= std::bind(&DesktopStreamer::_on_meta_size, this, _1);
		}
	};
	~DesktopStreamer(){ close(); };

	// should be atomic
	void open(){
		static bool is_reopen = false;
		if(_is_open) return;
		_is_open = true;
		if(is_reopen){
			using namespace smns::io;
			_app = Sock(Domain::INET, Type::TCP)
				.addr(_app.addr());
		} else is_reopen = true;

		_app.bind();
		std::cout << WSAGetLastError() << std::endl;
		assert(!_app.is_error() && "_app.bind() error");
		_app.listen(1);
		_threads.push_back(
			std::thread(&DesktopStreamer::_capturer, this));
		_threads.push_back(
			std::thread(&DesktopStreamer::_acceptor, this));
		_log("Ready to accept.");
	}

	// should be atomic
	void close(){
		if(!_is_open) return;
		_is_open = false;
		_app.close();
		_until_client_ready.notify_all();
		for(auto& thread : _threads)
			thread.join();
		_threads.clear();
	}

private:
	std::vector<std::thread> _threads;
	bool _is_open;
	Sock _app;
	Cmds _cmnds;
	DBuf _out_buff;

	#ifdef DEBUG
	Mtx _log_lock;
	#endif

	// wait capturer unitl
	// client connected
	bool _is_connected;
	Mtx _capture_lock;
	Cond _until_connected;

	// wait stream until
	// client ready
	bool _is_client_ready;
	Mtx _stream_lock;
	Cond _until_client_ready;

	// block callee only
	void _print(const char msg[], bool do_linefeed = false){
		std::cout << msg;
	}
	void _log(const char msg[]){
		#ifdef DEBUG
		std::lock_guard<Mtx> lock(_log_lock);
		std::clog << msg << std::endl;
		#endif
	}
	void _acceptor(){
		while(_is_open){
			Sock conn = _app.accept();

			// is app closed?
			// then break
			if(conn.val() < 0) break;
			else _log("New connection accepted.");
			_is_connected = true;
			std::thread(&DesktopStreamer::_receiver,
						this, conn).detach();
			// blocking
			_stream_to(conn);

			conn.close();
		}
		_log("Acceptor down.");
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
			if(temp.size() == 0){
				std::unique_lock<Mtx> lock(_stream_lock);
				_is_client_ready = true;
				continue;
			}

			// Warning!
			// The program will not operate nomally
			// when the file size  exceeds 2^32(=4G) byte.
			// Yes. It will not happen.
			// If it happens, increase size of 'size' header.
			*(reinterpret_cast<uint32_t*>(size.data()))
				= static_cast<uint32_t>(temp.size());

			if(conn.send(size).is_error()) break;
			if(conn.send(temp).is_error()) break;
		}
	}
	void _receiver(Sock& conn){
		Buff rcv_buff;
		while(_is_open){
			conn.recv(rcv_buff, 1);
			const uint8_t cmnd = rcv_buff[0];

			// undefined command
			if(cmnd >= DesktopStreamerCmd::_SIZE)
				continue;

			_cmnds[cmnd](conn);

			if(cmnd == DesktopStreamerCmd::DISCONNECT){
				_is_connected = false;
				break;
			}
		}
		_log("Disconnected");
	}
	void _capturer(){
		Mat shot;
		Buff encoded;
		_out_buff.open();
		while(_is_open){
			if(!_is_connected){
				std::unique_lock<Mtx> lock(_capture_lock);
				_until_client_ready.wait(lock, [this]{
					return _is_connected || !_is_open;
				});
			}
			smns::util::screen::capture(shot);
			cv::imencode(".jpg", shot, encoded, {
								CV_IMWRITE_JPEG_QUALITY, 80 // quality factor
			});
			_out_buff.push(std::move(encoded));
		}
		_out_buff.close();
		_log("Capturer down.");
	}

	//
	// commands
	//
	void _on_disconnect(Sock& conn){
		// release waiter
		_on_ready(conn);
	}
	void _on_ready(Sock&){
		{
			std::lock_guard<Mtx> lock(_stream_lock);
			_is_client_ready = true;
		}
		_until_client_ready.notify_all();
	}
	void _on_meta_size(Sock& conn){
		static Buff buff;
		static auto divide = [](auto lh, auto rh){
			return static_cast<float>(lh)
				/ static_cast<float>(rh);
		};
		conn.recv(buff, 4);
		const auto data = *(reinterpret_cast<uint32_t*>(buff.data()));
		const auto src_s = smns::util::screen::src_size();

		const auto src_w = src_s.right;
		const auto src_h = src_s.bottom;
		const auto dst_w = data >> 16;
		const auto dst_h = data & 0xFFFF;

		const auto w_ratio = divide(dst_w, src_w);
		const auto h_ratio = divide(dst_h, src_h);

		if(w_ratio > 1 && h_ratio > 1) return;

		smns::util::screen::resize(std::min(w_ratio, h_ratio));
	}
};