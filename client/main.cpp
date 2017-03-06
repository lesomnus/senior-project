#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <string>
#include <thread>
#include <opencv2/opencv.hpp>
#include <smns/DoubleBuffer.hpp>

#include "DesktopStreamRcver.hpp"
#include "Coupler.hpp"
#include "Player.hpp"

constexpr char		DFT_IP[] = "127.0.0.1";
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

	cv::namedWindow(TRG_WIN_NAME, CV_WINDOW_AUTOSIZE);
	//cv::setWindowProperty(_winname, CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);

	DesktopStreamRcver rcver(app_addr);
	Coupler coupler;
	Player player(TRG_WIN_NAME);

	rcver.open();
	rcver.pipe(coupler);
	coupler.pipe(player);



	bool power = true;
	print_help();
	// I hard-coded because there are not many commands.
	// Or you can use a Boost.Program_options to pull-request.
	std::thread listener([&coupler](bool& power){
		using namespace std;
		string line;

		while(power){
			cout << "> ";
			getline(std::cin, line);

			if(line == "on"){
				coupler.attach();
			} else if(line == "off"){
				coupler.detach();
			} else if(line == "exit"){
				power = false;
			} else if(line == "help"){
				print_help();
			} else{
				cout << "Unknown command" << endl;
			}
		}
	}, std::ref(power));

	while(power){ cv::waitKey(1); }



	listener.join();
	coupler.close();
	rcver.close();



	return 0;
}