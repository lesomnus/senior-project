#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <string>
#include <thread>
#include <condition_variable>
#include <opencv2/opencv.hpp>
#include <smns/DoubleBuffer.hpp>

#include "DesktopStreamRcver.hpp"
#include "Coupler.hpp"
#include "Player.hpp"

constexpr char		DFT_IP[] = "165.229.90.149";
constexpr uint16_t	DFT_PORT = 51220;
constexpr char		TRG_WIN_NAME[] = "remote desktop";

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
	#elif  (defined(LINUX) || defined(__linux__))
	cv::namedWindow(TRG_WIN_NAME, CV_WINDOW_AUTOSIZE);
	cv::setWindowProperty(TRG_WIN_NAME, CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
	std::cout << "asdfqqqqqqq" << std::endl;
	#endif

	DesktopStreamRcver rcver(app_addr);
	Coupler coupler;
	Player player(TRG_WIN_NAME);

	std::cout << "asdf" << std::endl;
	rcver.open();
	std::cout << "asdf" << std::endl;
	rcver.pipe(coupler);
	std::cout << "asdf" << std::endl;
	coupler.pipe(player);
	std::cout << "asdf" << std::endl;



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