#pragma once
#include <array>
#include <opencv2/opencv.hpp>

// based backfoward warpping
// only project to horizon rectangle
// dst points are calculated half of outter of src

class PerspectiveProjector{
private:
	using calc_t = double;
	using mtrx_t = cv::Mat_<calc_t>;
public:
	using elem_t = cv::Vec3b;
	using points = std::array<cv::Point, 4>;

	PerspectiveProjector(const points& src)
		:PerspectiveProjector(){
		eval(src);
	}
	PerspectiveProjector():
		_homography(3, 3){};

	void auto_init(cv::VideoCapture& cap){
		using namespace cv;
		constexpr const char AUTO_INIT_WINDOW_NAME[] = "_pattern";

		Mat shot;
		namedWindow(AUTO_INIT_WINDOW_NAME, CV_WINDOW_NORMAL);
		setWindowProperty(AUTO_INIT_WINDOW_NAME,
						  CV_WND_PROP_FULLSCREEN,
						  CV_WINDOW_FULLSCREEN);
		imshow(AUTO_INIT_WINDOW_NAME, _pattern);
		waitKey(2000);

		cap >> shot;
		cap >> shot;
		imshow("asdf", shot);
		waitKey(0);

		Mat projected_area = find_projected_area(shot);
		imshow("projected_area", projected_area);
		waitKey(0);
		points corners = find_corners(projected_area);	// false corners
		eval(corners);
		corners = scale_from_center({
			Point(0,0), Point(_bound.width, 0), Point(0, _bound.height),
			Point(_bound.width, _bound.height)
		}, 4.0 / 3);
		for(auto& corner : corners){
			corner = (*this)(corner);
		}
		eval(corners);

		destroyWindow(AUTO_INIT_WINDOW_NAME);
	}

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
	static cv::Mat _pattern;
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

	// 0 1
	// 2 3
	static points find_corners(const cv::Mat& src){
		using namespace std;
		using namespace cv;
		enum LOC{ LT, RT, LB, RB };

		constexpr float thresh = 150;
		const Mat& gray = src; //cvtColor(src, gray, COLOR_BGR2GRAY);
		Mat harris;
		{	// harris corner params
			constexpr int		max_thresh = 255;
			constexpr int		blockSize = 3;
			constexpr int		ksize = 3;
			constexpr double	k = 0.04;
			cornerHarris(gray, harris, blockSize, ksize, k);
		}

		Mat dst_norm; normalize(harris, dst_norm, 0, 255, NORM_MINMAX, CV_32FC1);
		Mat dst_norm_scaled; convertScaleAbs(dst_norm, dst_norm_scaled);

		vector<Point> corners;
		{
			const auto size = dst_norm.size();
			for(auto y = 0; y < size.height; ++y){
				for(auto x = 0; x < size.width; ++x){
					if(dst_norm.at<float>(y, x) < thresh) continue;
					corners.push_back(Point(x, y));
				}
			}
		}

		int x_min, x_max, y_min, y_max;
		{
			x_min = x_max = corners[0].x;
			y_min = y_max = corners[0].y;

			for(const auto& point : corners){
				x_min = min(x_min, point.x);
				x_max = max(x_max, point.x);
				y_min = min(y_min, point.y);
				y_max = max(y_max, point.y);
			}
		}

		array<Point, 4> rst;
		{
			array<vector<Point>, 4> set;
			{
				auto x_thrsh = (x_max + x_min) / 2;
				auto y_thrsh = (y_max + y_min) / 2;
				for(const auto& point : corners){
					uint8_t dtor = 0;
					dtor |= point.x > x_thrsh ? RT : 0;
					dtor |= point.y > y_thrsh ? LB : 0;

					set[dtor].push_back(point);
				}
			}

			for(auto i = 0; i < 4; ++i){
				const auto& set_ = set[i];
				auto x_sum = 0;
				auto y_sum = 0;
				for(const auto& point : set_){
					x_sum += point.x;
					y_sum += point.y;
				}
				rst[i] = Point(
					x_sum / set_.size(),
					y_sum / set_.size());
			}
		}

		return rst;
	}
	
	// target of src should be red (0,0,FF) outer rectangle
	// and green (0,FF,0) inner rectangle
	static cv::Mat find_projected_area(const cv::Mat& src){
		using namespace cv;

		auto contains = [](const Mat& src, const Point& trg){
			return trg.x > 0 || trg.y > 0
				|| trg.x < src.cols
				|| trg.y < src.rows;
		};

		enum FLAG: uint8_t{ IDLE = 0, NOT = 128, TRG = 255 };
		enum BGR: uint8_t{ B, G, R };

		const auto src_s = src.size();
		Mat rst(src_s, CV_8UC1); rst = Scalar(0);
		std::queue<Point> path;
		std::array<Point, 4> trgs;

		// initial path
		path.push(Point(src_s.width / 2, src_s.height / 2));

		// BFS
		while(!path.empty()){
			const auto cur = path.front();
			trgs = {
				Point(cur.x, cur.y - 1),
				Point(cur.x + 1, cur.y),
				Point(cur.x, cur.y + 1),
				Point(cur.x - 1, cur.y)
			};

			// traversal
			for(const auto& trg : trgs){
				if(!contains(src, trg)				// out of range
				   || rst.at<uint8_t>(trg) != IDLE	// already visited
				   ) continue;

				// visit
				const auto pix = src.at<Vec3b>(trg);
				if(pix[G] > pix[R]){
					rst.at<uint8_t>(trg) = TRG;
					path.push(trg);
				} else rst.at<uint8_t>(trg) = NOT;
			}

			path.pop();
		}

		return rst;
	}

	static points scale_from_center(const points& src, double scale){
		using namespace std;
		using namespace cv;

		Point center;
		{
			for(const auto& point : src){
				center.x += point.x;
				center.y += point.y;
			}
			center.x = static_cast<int>(center.x / 4);
			center.y = static_cast<int>(center.y / 4);
		}

		auto rst = src;
		{
			auto f = [scale](auto dst, auto ctr){ return (dst - ctr) * scale + ctr; };
			for(auto& point : rst){
				point = Point(
					f(point.x, center.x),
					f(point.y, center.y));
			}
		}

		return rst;
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

cv::Mat PerspectiveProjector::_pattern = [](){
	using namespace cv;
	Mat pattern(Size(8, 8), CV_8UC3);
	pattern = Scalar(0, 0, 255);
	rectangle(pattern, Rect(1, 1, 6, 6), Scalar(0, 255, 0), -1);

	return pattern;
}();