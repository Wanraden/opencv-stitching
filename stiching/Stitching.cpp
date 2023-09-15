#include "Stitching.h"

Stitching::Stitching(std::shared_ptr<FileList> pFilelist)
	: _stitcher(cv::Stitcher::createDefault(false))
	, bInit(false)
{
	fileList = pFilelist;
}

Stitching::~Stitching()
{
}

void Stitching::SetParameter(int from, int to, char flag, int Hessian, std::string resName, double cut, double invalid)
{
	if (fileList == nullptr)
		return;

	dilation_size = erosion_size = 1;

	minHessian = Hessian;
	start = from - 1;
	auto filesSize = (int)fileList->getCount();
	finish = filesSize >= to ? to - 1 : filesSize - 1;
	pic_num = to - from + 1;

	if (flag == 'Y')
		morph_if = true;
	else if (flag == 'N')
		morph_if = false;

	auto fileCount = fileList->getCount();
	imgsSize.resize(fileCount);
	imgs.resize(fileCount);
	mid_img.resize(fileCount);

	anchorY.resize(fileCount);
	for (size_t i = 0; i < fileCount; i++)
		anchorY[i] = 0;

	status_code = 10;
	resultName = resName;
	CUT = 0;
	overlapRatio = cut;
	invalidRatio = invalid;

	bInit = true;
}

int Stitching::LoadImages()
{
	// snip and imread >> roi[i]
	cv::Mat img;
	int erosion_type = cv::MORPH_RECT;
	cv::Mat erode_element = cv::getStructuringElement(erosion_type,
		cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1),
		cv::Point(erosion_size, erosion_size));

	for (int i = start; i <= finish; i++)
	{
		const std::string& filename = fileList->getIndexPath(i);  // const is a must!!!
		img = cv::imread(filename);
		if (img.empty())
			return i + 1; // i: 返回找不到的图片序号

		imgsSize[i] = { img.cols, (int)((double)img.rows * (1 - invalidRatio)) };
		cv::Rect snip(0, 0, imgsSize[i].width, imgsSize[i].height);
		if (morph_if == true)
		{
			cv::Mat erosion;
			img = cv::Mat(img, snip);
			cv::erode(img, erosion, erode_element);
			imgs[i] = erosion.clone();
		}
		else
			imgs[i] = cv::Mat(img, snip);
	}
	return 0; //0: 正常
}

int Stitching::diStitching(cv::Mat img1, cv::Mat img2, bool is_first) {
	//-- Step 1: Detect the keypoints using SURF Detector
	cv::SurfFeatureDetector detector(minHessian); // Very important to set minHessian 

	detector.detect(img1, keypoints_1);
	detector.detect(img2, keypoints_2);

	//-- Step 2: Calculate descriptors (feature vectors)
	cv::Mat descriptors_1, descriptors_2;
	extractor.compute(img1, keypoints_1, descriptors_1);
	extractor.compute(img2, keypoints_2, descriptors_2);

	//-- Step 3: Matching descriptor vectors using FLANN matcher
	// 这里对xx.jpg出错
	matcher.match(descriptors_1, descriptors_2, matches);

	//-- Quick calculation of max and min distances between keypoints
	double max_dist = 0, min_dist = 100;
	for (int i = 0; i < descriptors_1.rows; i++)
	{
		double dist = matches[i].distance;
		if (dist < min_dist) min_dist = dist;
		if (dist > max_dist) max_dist = dist;
	}

	//  test
	std::cout << "特征点信息距离" << std::endl;
	printf("-- Max dist : %f \n", max_dist);
	printf("-- Min dist : %f \n", min_dist);

	//-- Draw only "good" matches (i.e. whose distance is less than 2*min_dist,
	//-- or a small arbitary value ( 0.02 ) in the event that min_dist is very
	//-- small)
	//-- PS.- radiusMatch can also be used here.

	double ratio_threshold = 2.0f;
	for (int i = 0; i < descriptors_1.rows; i++)
	{
		if (matches[i].distance < std::max(ratio_threshold * min_dist, 0.02))
		{
			good_matches.push_back(matches[i]);
		}
	}

	std::vector<cv::Mat>().swap(double_img); // must clear the double_img's vector!!!
	double_img.push_back(img1);
	double_img.push_back(img2);

	CUT = (int)(overlapRatio * img2.cols);
	cv::Rect roi2(0, 0, CUT, img2.rows);
	std::vector<cv::Rect> left, right;
	if (is_first == true) {
		cv::Rect roi1(img1.cols - CUT, 0, CUT, img1.rows);
		left.push_back(roi1);
	}
	else {
		cv::Rect roi1(0, 0, img1.cols, img1.rows);
		left.push_back(roi1);
	}
	right.push_back(roi2);
	std::vector<std::vector<cv::Rect>> rois;
	rois.push_back(left);
	rois.push_back(right);

	//Stitcher::Status Stitcher::estimateTransform(InputArray images, const vector<vector<cv::Rect> > &rois)
	//	//images表示待拼接的输入图像
	//	//rois表示输入图像中感兴趣的矩形区域，即只对该区域进行拼接
	//{
	//	images.getMatVector(imgs_);    //图像赋值
	//	rois_ = rois;    //赋值
	//	Status status;
	//	//调用matchImages函数，用于匹配图像，该函数在后面给出介绍
	//	if ((status = matchImages()) != OK)
	//		return status;
	//	//调用estimateCameraParams函数，用于评估相机参数，该函数在后面给出介绍
	//	estimateCameraParams();
	//	return OK;
	//}

	cv::Mat pano;
	cv::Stitcher::Status status = _stitcher.stitch(double_img, rois, pano);
	if (status != cv::Stitcher::OK) 
		return -1;

	mid = pano.clone();

	double sum_x1 = 0.0, sum_y1 = 0.0, sum_x2 = 0.0, sum_y2 = 0.0;
	for (int i = 0; i < (int)good_matches.size(); i++)
	{
		sum_x1 += keypoints_1[good_matches[i].queryIdx].pt.x;
		sum_y1 += keypoints_1[good_matches[i].queryIdx].pt.y;
		sum_x2 += keypoints_2[good_matches[i].trainIdx].pt.x;
		sum_y2 += keypoints_2[good_matches[i].trainIdx].pt.y;
	}
	int sum = (int)good_matches.size();

	keyPoint_x1 = sum_x1 / sum;
	keyPoint_y1 = sum_y1 / sum;
	keyPoint_x2 = sum_x2 / sum;
	keyPoint_y2 = sum_y2 / sum;

	good_matches.clear();
	matches.clear();

	std::cout << "成功拼接并存储！" << std::endl;
	return 0; // 0: 表示两张图片拼接正确！
}

int Stitching::midProcess() {
	// imgs' channel = 3
	// stitch and acquire mid_img[i]
	cv::Mat temp = imgs[start].clone();
	for (int i = start; i <= finish - 1; i++) {
		int diStatus;
		if (i == start) {
			diStatus = diStitching(temp, imgs[i + 1], true);
		}
		else {
			diStatus = diStitching(temp, imgs[i + 1], false);
		}
		if (diStatus == -1) {
			std::cout << "两张图片拼接出现问题！" << std::endl;
			status_code = 0;
			return i + 1;
		}
		if (pic_num == 2) {
			result = mid.clone();
			return 0; // 0: all green
		}
		// 判断是否为相邻重复图片
		if (keyPoint_x2 >= (double)(imgsSize[i].width - CUT)) {
			std::cout << "Img_" << i + 1 << " & Img_" << i + 2 << " coincides!" << std::endl;
			status_code = 1; // 1: coincide
			return i + 1;
		}
		// cols是宽度, rows是高度
		//imwrite("mid_" + to_string(i) + ".tif", mid);

		// expand
		if (i > 0 && i != start && mid.rows < mid_img[i - 1].rows) {
			//cv::Mat声明语句 cv::Mat(int rows, int cols, int type, const Scalar& s);
			cv::Mat expansion(mid_img[i - 1].rows, mid.cols, mid.type(), cv::Scalar(0, 0, 0));
			cv::Mat img_roi = expansion(cv::Rect(0, 0, mid.cols, mid.rows));
			mid.copyTo(img_roi);
			mid = expansion.clone();
			expansion.release();
		}

		if (i == finish - 1) {
			anchorY[i] = abs((int)(mid.rows - mid_img[i - 1].rows));
			if ((keyPoint_y1 - keyPoint_y2) < 0) anchorY[i] = -anchorY[i];
			mid_img[finish - 1] = mid.clone();
			std::cout << "第" << i + 1 << "张图片与" << "第" << (i + 2) << "张图片成功拼接！" << std::endl;
			//imwrite("mid_img_" + to_string(i) + ".tif", mid_img[i]);
			std::cout << std::endl;
			for (int j = start + 1; j <= finish - 1; j++)
				std::cout << anchorY[j] << " ";
			std::cout << std::endl;
			break;
		}
		if (i != start) {
			anchorY[i] = abs((int)(mid.rows - mid_img[i - 1].rows));
			if ((keyPoint_y1 - keyPoint_y2) < 0) anchorY[i] = -anchorY[i];
			//midResult(i);
		}
		cv::Rect _cut(0, 0, mid.cols - CUT, mid.rows);
		cv::Rect cut_(mid.cols - CUT, 0, CUT, mid.rows);
		mid_img[i] = cv::Mat(mid, _cut);
		temp = cv::Mat(mid, cut_);
		//imwrite("mid_img_" + std::to_string(i) + ".tif", mid_img[i]);
		std::cout << "第" << i + 1 << "张图片与" << "第" << (i + 2) << "张图片成功拼接！" << std::endl;
	}

	std::cout << "中间结果计算完毕！" << std::endl;
	for (int i = start; i <= finish - 1; i++)
		width += mid_img[i].cols;
	return 0;
}

int Stitching::midProcessStatus() {
	return status_code;
}

int Stitching::midResult(int pos)
{
	if (pic_num == 2) {
		return -1;
	}
	int widthMid = 0;
	for (int i = start; i <= pos - 1; i++)
		widthMid += mid_img[i].cols;
	auto midres_img = mid_img;
	cv::Mat dst(midres_img[pos - 1].rows, widthMid, imgs[start].type(), cv::Scalar(0, 0, 0));

	cv::Mat img_roi = dst(cv::Rect(widthMid - midres_img[pos - 1].cols, 0, midres_img[pos - 1].cols, midres_img[pos - 1].rows));

	midres_img[pos - 1].copyTo(img_roi);
	img_roi.release();

	int sum_width = midres_img[pos - 1].cols;
	int sum_height = 0;
	for (size_t i = pos - 1; i >= start + 1; i--) {
		sum_width += midres_img[i - 1].cols;
		if (anchorY[i] > 0) {
			cv::Mat img_roi = dst(cv::Rect(widthMid - sum_width, sum_height, midres_img[i - 1].cols, midres_img[i - 1].rows));
			midres_img[i - 1].copyTo(img_roi);
			img_roi.release();
		}
		else if (anchorY[i] <= 0) {
			sum_height -= anchorY[i];
			cv::Mat img_roi = dst(cv::Rect(widthMid - sum_width, sum_height, midres_img[i - 1].cols, midres_img[i - 1].rows));
			midres_img[i - 1].copyTo(img_roi);
			img_roi.release();
		}
	}
	imwrite(resultName, dst);
	return 0;
}

int Stitching::FinalWarp() {
	//cv::Mat声明语句 cv::Mat(int rows, int cols, int type, const Scalar& s);  
	//imgs[start].type() = CV_8UC3
	if (pic_num == 2) {
		return -1;
	}
	cv::Mat dst(mid_img[finish - 1].rows, width, imgs[start].type(), cv::Scalar(0, 0, 0));

	cv::Mat img_roi = dst(cv::Rect(width - mid_img[finish - 1].cols, 0, mid_img[finish - 1].cols, mid_img[finish - 1].rows));

	mid_img[finish - 1].copyTo(img_roi);
	img_roi.release();

	int sum_width = mid_img[finish - 1].cols;
	int sum_height = 0;
	for (size_t i = finish - 1; i >= start + 1; i--) {
		sum_width += mid_img[i - 1].cols;
		if (anchorY[i] > 0) {
			cv::Mat img_roi = dst(cv::Rect(width - sum_width, sum_height, mid_img[i - 1].cols, mid_img[i - 1].rows));
			mid_img[i - 1].copyTo(img_roi);
			img_roi.release();
		}
		else if (anchorY[i] <= 0) {
			sum_height -= anchorY[i];
			cv::Mat img_roi = dst(cv::Rect(width - sum_width, sum_height, mid_img[i - 1].cols, mid_img[i - 1].rows));
			mid_img[i - 1].copyTo(img_roi);
			img_roi.release();
		}
	}
	result = dst.clone();
	return 0;
}

void Stitching::Result()
{
	int dilation_type = cv::MORPH_RECT;
	cv::Mat dilation_element = cv::getStructuringElement(dilation_type,
		cv::Size(2 * dilation_size + 1, 2 * dilation_size + 1),
		cv::Point(dilation_size, dilation_size));
	if (morph_if == false) {
		imwrite(resultName, result);
	}
	else {
		cv::Mat dilation;
		dilate(result, dilation, dilation_element);
		imwrite(resultName, dilation);
	}
}

void Stitching::Clear()
{
	imgsSize.clear();
	imgs.clear();
	mid_img.clear();
	anchorY.clear();
	auto fileCount = fileList->getCount();
	imgsSize.resize(fileCount);
	imgs.resize(fileCount);
	mid_img.resize(fileCount);

	anchorY.resize(fileCount);
	for (size_t i = 0; i < fileCount; i++)
		anchorY[i] = 0;


	std::vector<cv::DMatch>().swap(matches);
	std::vector<cv::DMatch>().swap(good_matches);
	std::vector<cv::Mat>().swap(double_img);

	std::vector<cv::KeyPoint>().swap(keypoints_1);
	std::vector<cv::KeyPoint>().swap(keypoints_2);
	width = 0;
}

void Stitching::Start()
{
	if (!bInit)
	{
		std::cout << "未初始化，使用SetParameter初始化" << std::endl;
		Clear();
		return;
	}
	auto res = LoadImages();
	if (res != 0)
	{
		std::cout << "第 " << res << " 张图加载失败" << std::endl;
		Clear();
		return;
	}
	res = midProcess();
	if (res != 0)
	{
		std::cout << "第 " << res << " 张图拼接失败" << std::endl;
		Clear();
		return;
	}
	res = FinalWarp();
	Result(); // 保存结果图片
	Clear();
}
