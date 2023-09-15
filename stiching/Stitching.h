#pragma once

#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/nonfree/features2d.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/nonfree/nonfree.hpp>
#include <opencv2/legacy/legacy.hpp>
#include <opencv2/stitching/stitcher.hpp>
#include "FileList.h"

class Stitching
{
public:
	Stitching(std::shared_ptr<FileList> pFilelist);
	~Stitching();
	void SetParameter(int from, int to, char flag, int Hessian, std::string res, double cut, double invalid);
	void Start();

protected:
	int LoadImages();
	int diStitching(cv::Mat img1, cv::Mat img2, bool is_first);
	int midProcess();
	int midProcessStatus();
	int midResult(int pos);
	int FinalWarp();
	void Result();
	void Clear();

private:
	bool bInit;
	std::shared_ptr<FileList> fileList; // 文件路径管理

	cv::Stitcher _stitcher; // 

	std::string resultName;
	int status_code;     // midProcess -> 0: 不相干图片, 1: coincide
	int start;
	int finish;
	int pic_num;
	int width;           // 最后结果的长度
	int erosion_size;
	int dilation_size;

	bool morph_if;       // 调用erode，用途未知，暂时为false
	double overlapRatio; // 图像左右重叠区域比例
	double invalidRatio; // 图像底部信息区域比例
	int CUT;             // 图像左右重叠区域宽度

	int minHessian;      // 阈值，如何确定阈值?
	double keyPoint_x1;
	double keyPoint_y1;
	double keyPoint_x2;
	double keyPoint_y2;

	cv::Mat mid;
	cv::Mat result;
	std::vector<cv::Size> imgsSize;
	std::vector <cv::Mat> imgs;
	std::vector <cv::Mat> mid_img;
	std::vector<int> anchorY;

	cv::SurfDescriptorExtractor extractor;

	cv::FlannBasedMatcher   matcher;
	std::vector<cv::DMatch> matches;
	std::vector<cv::DMatch> good_matches;
	std::vector<cv::Mat>    double_img;

	std::vector<cv::KeyPoint> keypoints_1;
	std::vector<cv::KeyPoint> keypoints_2;
};

