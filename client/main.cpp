/**
* File Name:
*	client/main.cpp
* Description:
*	Main procedure for application.
*	Setting network configuration and give it to "DesktopStreamRcver" instance.
*	Initiate "DesktopStreamRcver", "Coupler" and "Player" then pipe them.
*	Provide on or off the "ImageProcessor" and exit command.
*
* Programmed by Hwang Seung Huyn
* Check the version control of this file
* here-> https://github.com/lesomnus/senior-project/commits/master/client/main.cpp
*/

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <string>
#include <thread>
#include <condition_variable>
#include <opencv2/opencv.hpp>
#include <smns/DoubleBuffer.hpp>

#include "PerspectiveProjector.hpp"
#include "DesktopStreamRcver.hpp"
#include "Coupler.hpp"
#include "Player.hpp"

constexpr char		DFT_IP[] = "127.0.0.1";
constexpr uint16_t	DFT_PORT = 51220;
constexpr char		TRG_WIN_NAME[] = "remote desktop";

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
	f("on", "turn on image processor");
	f("off", "turn off image processor");
	f("exit", "exit program");
	f("help", "display this help");
	cout << "-----------------------------------" << endl;
}

int main(){
	auto app_addr = smns::io::Addr()
		.domain(smns::io::Domain::INET)
		.ip(DFT_IP).port(DFT_PORT);

	#if defined(_WIN32)
	cv::namedWindow(TRG_WIN_NAME, CV_WINDOW_NORMAL);
	//cvSetWindowProperty(TRG_WIN_NAME, CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
	#elif  (defined(LINUX) || defined(__linux__))
	cv::namedWindow(TRG_WIN_NAME, CV_WINDOW_AUTOSIZE);
	cv::setWindowProperty(TRG_WIN_NAME, CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
	#endif

	DesktopStreamRcver rcver(app_addr);
	Coupler coupler;
	Player player(TRG_WIN_NAME);

	rcver.open();
	rcver.pipe(coupler);
	coupler.pipe(player);



	bool power = true;
	std::mutex power_lock;
	std::condition_variable until_power_off;
	print_help();
	// I hard-coded because there are not many commands.
	// Or you can use a Boost.Program_options to pull-request.
	std::thread listener([&coupler, &power_lock, &until_power_off](bool& power){
		using namespace std;
		string line;

		while(true){
			cout << "> ";
			getline(std::cin, line);

			if(cin.fail() || cin.eof()){ line = "exit"; }
			if(line == "on"){
				coupler.attach();
			} else if(line == "off"){
				coupler.detach();
			} else if(line == "exit"){
				std::lock_guard<std::mutex> lock(power_lock);
				power = false;
				break;
			} else if(line == "help"){
				print_help();
			} else{
				cout << "Unknown command" << endl;
			}
		}
		until_power_off.notify_one();
	}, std::ref(power));

	#if defined(_WIN32)
	while(true){
		std::lock_guard<std::mutex> lock(power_lock);
		if(!power) break;
		cv::waitKey(20);
	}
	#elif (defined(LINUX) || defined(__linux__))
	{
		std::unique_lock<std::mutex> lock(power_lock);
		until_power_off.wait(lock, [&power]{ return !power; });
	}
	#endif




	listener.join();
	std::cout << "listener down" << std::endl;
	rcver.close();
	std::cout << "receiver down" << std::endl;
	coupler.close();
	std::cout << "coupler down" << std::endl;



	return 0;
}