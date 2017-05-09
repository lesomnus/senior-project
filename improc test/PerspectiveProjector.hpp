#pragma once
#include <array>
#include <opencv2/opencv.hpp>

// based backward warpping
// only project to horizon rectangle
// dst points are calculated half of outter of src

template<typename Telem = cv::Vec3b>
class PerspectiveProjector{
private:
	using calc_t = double;
	using mtrx_t = cv::Mat_<calc_t>;
public:
	using elem_t = Telem;
	using points = std::array<cv::Point, 4>;

	PerspectiveProjector(const points& src)
		:PerspectiveProjector(){
		eval(src);
	}
	PerspectiveProjector():
		_homography(3, 3){};



	void eval(const points& src){
		using namespace cv;
		_bound = _outer_of(src);
		const points dst = {
			Point(0,0),
			Point(_bound.width, 0),
			Point(0, _bound.height),
			Point(_bound.width, _bound.height)
		};
		mtrx_t src_(8, 8);
		mtrx_t dst_(8, 1);
		mtrx_t trg_(8, 1);


		src_ <<
			dst[0].x, dst[0].y, 1, 0, 0, 0, -(dst[0].x * src[0].x), -(dst[0].y * src[0].x),
			0, 0, 0, dst[0].x, dst[0].y, 1, -(dst[0].x * src[0].y), -(dst[0].y * src[0].y),
			dst[1].x, dst[1].y, 1, 0, 0, 0, -(dst[1].x * src[1].x), -(dst[1].y * src[1].x),
			0, 0, 0, dst[1].x, dst[1].y, 1, -(dst[1].x * src[1].y), -(dst[1].y * src[1].y),
			dst[2].x, dst[2].y, 1, 0, 0, 0, -(dst[2].x * src[2].x), -(dst[2].y * src[2].x),
			0, 0, 0, dst[2].x, dst[2].y, 1, -(dst[2].x * src[2].y), -(dst[2].y * src[2].y),
			dst[3].x, dst[3].y, 1, 0, 0, 0, -(dst[3].x * src[3].x), -(dst[3].y * src[3].x),
			0, 0, 0, dst[3].x, dst[3].y, 1, -(dst[3].x * src[3].y), -(dst[3].y * src[3].y);
		dst_ <<
			src[0].x, src[0].y, src[1].x, src[1].y,
			src[2].x, src[2].y, src[3].x, src[3].y;
		trg_ = src_.inv() * dst_;

		_homography <<
			trg_(0, 0), trg_(1, 0), trg_(2, 0),
			trg_(3, 0), trg_(4, 0), trg_(5, 0),
			trg_(6, 0), trg_(7, 0), 1;
	}

	// I use backward warpping
	cv::Point operator()(cv::Point dst){
		mtrx_t src_pmat(3, 1);
		mtrx_t dst_pmat(3, 1);
		double w =
			_homography(2, 0) * dst.x +
			_homography(2, 1) * dst.y + 1;

		dst_pmat << dst.x, dst.y, 1;
		src_pmat = _homography * dst_pmat;

		return cv::Point(
			int(src_pmat(0, 0) / w),
			int(src_pmat(1, 0) / w));
	}
	cv::Mat operator()(cv::Mat& src){
		return _wrap(src);
	}

	mtrx_t homography(){ return _homography; }
	cv::Rect bound(){ return _bound; }

private:
	mtrx_t _homography;
	cv::Rect _bound;

	static cv::Rect _outer_of(const points& src){
		using namespace cv;
		Point from = src[0];
		Point to = from;

		for(auto i = 1; i < 4; ++i){
			from.x = std::min(from.x, src[i].x);
			from.y = std::min(from.y, src[i].y);
			to.x = std::max(to.x, src[i].x);
			to.y = std::max(to.y, src[i].y);
		}

		return Rect(from, to);
	}

	cv::Mat _wrap(const cv::Mat& src){
		using namespace cv;
		Mat dst(Size(_bound.width,
					 _bound.height),
				src.type());
		mtrx_t src_pmat(3, 1);
		mtrx_t dst_pmat(3, 1);

		for(auto y = 0; y < dst.rows; ++y){
			// careful
			elem_t* pix = dst.ptr<elem_t>(y);

			for(auto x = 0; x < dst.cols; ++x){
				dst_pmat << x, y, 1;
				src_pmat = _homography * dst_pmat;


				double w =
					_homography(2, 0) * x +
					_homography(2, 1) * y + 1;
				Point dst_p(x, y);
				Point src_p(
					int(src_pmat(0, 0) / w),
					int(src_pmat(1, 0) / w));

				if(!_bound.contains(src_p))
					continue;

				pix[x] = src.at<elem_t>(src_p);
			}
		}

		return dst;
	}
};