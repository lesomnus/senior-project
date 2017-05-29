/**
* File Name:
*	client/KinectCapture.hpp
* Description:
*	A capturer which capture the RGB or Depth images via Kinect.
*	Capture the image and tranform to cv::Mat class.
*
* Programmed by Hwang Seung Huyn
* Check the version control of this file
* here-> https://github.com/lesomnus/senior-project/commits/master/client/KinectCapture.hpp
*/

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

	/**
	*  Function Name: read
	*  Input arguments (condition):
	*	None.
	*  Processing in function (in pseudo code style):
	*	1) Read image data frome the buffer.
	*	2) Transform the image data to cv::Mat class then return it.
	*  Function Return: 
	*	None.
	*/
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
	static const cv::Size size;

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

		Mat rst_;
		cvtColor(Mat(KinectCapture::size,
					 CV_8UC4, _rgbWk,
					 Mat::AUTO_STEP),
				 rst_, CV_BGRA2GRAY);

		Mat rst = _project_to_color_cam(rst_);
		flip(rst, rst, 1);
		return rst;
	}
private:
	RGBQUAD* _rgbWk;
	static const cv::Mat_<double> _homor_to_color_cam;

	static cv::Mat _project_to_color_cam(const cv::Mat& src){
		using namespace cv;
		using elem_t = uint8_t;
		Size src_s = src.size();
		Mat rst(KinectCapture::size, src.type()); rst = Scalar(0);
		Mat_<double> src_pmat(3, 1);
		Mat_<double> dst_pmat(3, 1);
		Point dst_p, src_p;
		Rect bound(Point(0, 0), rst.size());

		for(auto y = 0; y < src_s.height; ++y){
			auto pix = rst.ptr<elem_t>(y);

			for(auto x = 0; x < src_s.width; ++x){
				dst_pmat << x, y, 1;
				src_pmat = _homor_to_color_cam * dst_pmat;

				double w =
					_homor_to_color_cam(2, 0) * x +
					_homor_to_color_cam(2, 1) * y + 1;
				dst_p = Point(x, y);
				src_p = Point(
					int(src_pmat(0, 0) / w - 8),
					int(src_pmat(1, 0) / w + 10));

				if(!bound.contains(src_p))
					continue;

				pix[x] = src.at<elem_t>(src_p);
			}
		}

		return rst;
	}

	RGBQUAD _short2quad(USHORT s){
		USHORT realDepth = (s & 0xfff8) >> 3;
		//플레이어 비트 정보가 필요할 경우 : USHORT Player = s & 7;
		BYTE l = 255 - (BYTE)(256 * realDepth / (0x0fff));
		RGBQUAD q;
		q.rgbRed = q.rgbBlue = q.rgbGreen = l;
		return q;
	}
};
const cv::Size KinectDepthCapture::size = cv::Size(640 - 8, 480);
const cv::Mat_<double> KinectDepthCapture::_homor_to_color_cam = ([](){
	cv::Mat_<double> val(3, 3);
	val <<
		1.105214746018476, -0.04681816932036342, -17.41963323570281,
		0.01183746927917436, 1.054422182952943, -28.25615493416862,
		3.048218215291311e-05, -0.0001258552938719446, 1;
	return val;
})();

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