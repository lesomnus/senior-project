/**
* File Name:
*	server/main.cpp
* Description:
*	Main procedure for application.
*	Setting network configuration and give it to "DesktopStreamer" instance.
*	Provide open, close and exit command.
*
* Programmed by Hwang Seung Huyn
* Check the version control of this file
* here-> https://github.com/lesomnus/senior-project/commits/master/server/main.cpp
*/

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <smns/io/addr.hpp>

#include "DesktopStreamer.hpp"

constexpr char*		DFT_IP = "127.0.0.1";
constexpr uint16_t	DFT_PORT = 51220;

/**
*  Function Name: void pirint_help()
*  Input arguments (condition):
*	None.
*  Processing in function (in pseudo code style):
*	1) Print menu.
*  Function Return: 
*	None.
*/
void print_help(){
	using namespace std;
	auto f = [](const auto& command,
				const auto& descript){
		cout << setw(8) << command << " : " << descript << endl;
	};
	cout << "-----------------------------------" << endl;
	f("open", "open streaming srever");
	f("close", "close streaming server");
	f("exit", "exit program");
	f("help", "display this help");
	cout << "-----------------------------------" << endl;
}

int main(int argc, char** argv){
	auto app_addr = smns::io::Addr()
		.domain(smns::io::Domain::INET)
		.ip(DFT_IP).port(DFT_PORT);

	DesktopStreamer app(app_addr);



	print_help();
	// I hard-coded because there are not many commands.
	// Or you can use a Boost.Program_options to pull-request.
	{
		using namespace std;
		string line;

		while(true){

			cout << "> ";
			getline(std::cin, line);

			if(line == "open"){
				app.open();
			} else if(line == "close"){
				app.close();
			} else if(line == "exit"){
				app.close(); break;
			} else if(line == "help"){
				print_help();
			} else{ cout << "Unknown command" << endl; }
		}
	}



	return 0;
}