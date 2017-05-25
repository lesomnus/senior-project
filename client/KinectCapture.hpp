#pragma once
#include <opencv2/opencv.hpp>
#define _WINSOCKAPI_
#include <windows.h>
#include <NuiApi.h>

class KinectCapture{
public:
	KinectCapture(){
		if(!_is_initiate){
			NuiInitialize(NUI_INITIALIZE_FLAG_USES_COLOR |
						  NUI_INITIALIZE_FLAG_USES_DEPTH);
			_is_initiate = true;
		}
	}

	static const cv::Size size;
	virtual cv::Mat read() = 0;
	KinectCapture& operator >> (cv::Mat& rst){
		rst = read();
		return *this;
	}

protected:
	static bool _is_initiate;
	HANDLE	_handle;
	BYTE*	_buffer;

	const NUI_IMAGE_FRAME*	_frame;
	INuiFrameTexture*		_texture;
	NUI_LOCKED_RECT			_locked_rect;
};
bool KinectCapture::_is_initiate = false;
const cv::Size KinectCapture::size = cv::Size(640, 480);

class KinectDepthCapture: public KinectCapture{
public:
	KinectDepthCapture(){
		NuiImageStreamOpen(NUI_IMAGE_TYPE_DEPTH,
						   NUI_IMAGE_RESOLUTION_640x480,
						   0, 2, nullptr, &_handle);
		_rgbWk = new RGBQUAD[640 * 480];
	}
	~KinectDepthCapture(){
		delete _rgbWk;
	}

	cv::Mat read(){
		using namespace cv;
		HRESULT hr;
		hr = NuiImageStreamGetNextFrame(_handle, 1000, &_frame);
		if(FAILED(hr)) return Mat();

		_texture = _frame->pFrameTexture;
		_texture->LockRect(0, &_locked_rect, nullptr, 0);

		hr = NuiImageStreamReleaseFrame(_handle, _frame);
		if(FAILED(hr) || (_locked_rect.Pitch == 0)) return Mat();

		_buffer = (BYTE*)_locked_rect.pBits;

		auto buff_run = (USHORT*)_buffer;
		auto quad_run = (RGBQUAD*)_rgbWk;
		for(auto y = 0; y < KinectCapture::size.height; ++y){
			for(auto x = 0; x < KinectCapture::size.width; ++x){
				*quad_run = _short2quad(*buff_run);
				buff_run++; quad_run++;
			}
		}

		Mat rst(KinectCapture::size, CV_8UC1);
		cvtColor(Mat(KinectCapture::size,
					 CV_8UC4, _rgbWk,
					 Mat::AUTO_STEP),
				 rst, CV_BGRA2GRAY);
		flip(rst, rst, 1);
		return rst;
	}
private:
	RGBQUAD* _rgbWk;

	RGBQUAD _short2quad(USHORT s){
		USHORT realDepth = (s & 0xfff8) >> 3;
		//플레이어 비트 정보가 필요할 경우 : USHORT Player = s & 7;
		BYTE l = 255 - (BYTE)(256 * realDepth / (0x0fff));
		RGBQUAD q;
		q.rgbRed = q.rgbBlue = q.rgbGreen = l;
		return q;
	}
};

class KinectColorCapture: public KinectCapture{
public:
	KinectColorCapture(){
		NuiImageStreamOpen(NUI_IMAGE_TYPE_COLOR,
						   NUI_IMAGE_RESOLUTION_640x480,
						   0, 2, nullptr, &_handle);
	}

	cv::Mat read(){
		using namespace cv;
		HRESULT hr;
		hr = NuiImageStreamGetNextFrame(_handle, 1000, &_frame);
		if(FAILED(hr)) return Mat();

		_texture = _frame->pFrameTexture;
		_texture->LockRect(0, &_locked_rect, nullptr, 0);

		hr = NuiImageStreamReleaseFrame(_handle, _frame);
		if(FAILED(hr) || (_locked_rect.Pitch == 0)) return Mat();

		_buffer = (BYTE*)_locked_rect.pBits;

		Mat rst;
		cvtColor(Mat(KinectCapture::size,
					 CV_8UC4, _buffer,
					 Mat::AUTO_STEP),
				 rst, CV_BGRA2BGR);
		flip(rst, rst, 1);
		return rst;
	}
};