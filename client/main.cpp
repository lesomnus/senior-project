#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <opencv2/opencv.hpp>

#include "DesktopStreamRcver.hpp"

constexpr char*		DFT_IP = "127.0.0.1";
constexpr uint16_t	DFT_PORT = 51220;

int main(){
	auto app_addr = smns::io::Addr()
		.domain(smns::io::Domain::INET)
		.ip(DFT_IP).port(DFT_PORT);
	DesktopStreamRcver rcver(app_addr);
	rcver.open();

	cv::Mat shot;
	while(true){
		shot = rcver.pop();
		if(shot.empty()) continue;

		cv::imshow("asdf", shot);
		char key = cv::waitKey(1);
		if(key == 'q') break;
	}


	return 0;
}