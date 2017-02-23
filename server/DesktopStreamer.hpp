#pragma once
#include <assert.h>
#include <string>
#include <thread>

#include <opencv2/opencv.hpp>
#include <smns/io/sock.hpp>
#include <smns/util/screen.hpp>

using namespace smns;

class DesktopStreamer{
private:
	using Str = std::string;
	using Addr = smns::io::Addr;
	using Sock = smns::io::Sock;

public:
	DesktopStreamer(Addr address)
		: _is_open(false), _app(io::Domain::INET, io::Type::TCP){
		_app.addr(address).bind();
		assert(!_app.is_error() && "_app.bind() error");
	};
	~DesktopStreamer(){};

	// should be atomic
	void open(){
		if(_is_open) return;
		_is_open = true;
		std::thread(&DesktopStreamer::_open, this).detach();
	}

	// should be atomic
	void close(){
		if(!_is_open) return;
		_is_open = false;
		_app.close();
	}

private:
	bool _is_open;
	Sock _app;

	static void _log(const char* msg){
		std::clog << msg << std::endl;
	}
	void _open(){
		_app.listen(1);
		_log("Ready to accept.");
		while(_is_open){
			Sock conn = _app.accept();
			
			// is app closed?
			if(conn.is_error()) break;
			else _log("New connection accepted");

			_stream();
			conn.close();
		}
	}
	void _stream(){

	}
};