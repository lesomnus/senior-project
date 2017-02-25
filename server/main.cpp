#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

#include <smns/io/addr.hpp>

#include "DesktopStreamer.hpp"

constexpr char*		DFT_IP = "127.0.0.1";
constexpr uint16_t	DFT_PORT = 51220;
constexpr double	DFT_SCREEN_RATIO = 0.4;

void print_help(){
	using namespace std;
	cout
		<< "-----------------------------------" << endl
		<< " open	open streaming srever" << endl
		<< " close	close streaming server" << endl
		<< " exit	exit program" << endl
		<< " help	display this help" << endl
		<< "-----------------------------------" << endl;
}

// There are no options, or one,
// or more than two, but up to three.
//	[init image size] or
//	[ip port [init image size]]
//
// Examples of options. 
//	165.229.90.149 51220 1
//		listen 165.229.90.149:51220 and
//		initiatial image size is the same as original.
//	0.8
//		initiatial image size is 0.8 times the original.
int main(int argc, char** argv){
	auto app_addr = smns::io::Addr()
		.domain(smns::io::Domain::INET)
		.ip(DFT_IP).port(DFT_PORT);
	smns::util::screen::resize(DFT_SCREEN_RATIO);
	{
		auto num_of_opt = std::min(argc - 1, 3);
		if(num_of_opt == 1){
			smns::util::screen::resize(std::atof(argv[1]));
		} else{
			switch(num_of_opt){
			case 3: smns::util::screen::resize(std::atof(argv[3]));
			case 2: app_addr.ip(argv[1]).port(std::atoi(argv[2]));
			default: break;
			}
		}
	}

	DesktopStreamer app(app_addr);
	print_help();
	std::string in_line;
	// I hard-coded because there are not many options.
	// Or you can use a Boost.Program_options to pull-request.
	while(true){
		std::cout << "> ";
		std::getline(std::cin, in_line);
		if(in_line == "open"){
			app.open();
		} else if(in_line == "close"){
			app.close();
		} else if(in_line == "exit"){
			app.close(); break;
		} else if(in_line == "help"){
			print_help();
		} else{
			std::cout << "unknown option" << std::endl;
		}
	}
	return 0;
}