// cv-demo3.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "Stitching.h"

void ReadImg()
{
	cv::Mat image;   //创建一个空图像image
	image = cv::imread(R"(D:\wkp\ex-soui\SouiWizard3\result1.tif)");  //读取文件夹中的图像

	//检测图像是否加载成功
	if (image.empty())  //检测image有无数据，无数据 image.empty()返回 真
	{
		std::cout << "Could not open or find the image" << std::endl;
		return;
	}

	cv::namedWindow("IMAGE");  //创建显示窗口，不加这行代码，也能显示，默认窗口大小不能改变
	cv::imshow("IMAGE", image);  //在窗口显示图像


	cv::waitKey(0);  //暂停，保持图像显示，等待按键结束
	return;
}

int SFImg()
{
    printf("\n Zoom In-Out demo \n");
    printf("-------------------- \n");
    printf("*[u]-> Zoom in \n");
    printf("*[d]-> Zoom out \n");
    printf("*[ESC]-> Close program \n\n");

    cv::Mat src, dst, tmp;
    const char* window_name = "Pyramids Demo";

    src = cv::imread(R"(D:\wkp\ex-soui\SouiWizard3\result1.tif)");
    if (!src.data)
    {
        printf("No data!--Exiting the program \n");
        return -1;
    }

    tmp = src;
    dst = tmp;
    cv::namedWindow(window_name, CV_WINDOW_AUTOSIZE);
    cv::imshow(window_name, dst);

    while (true)
    {
        int c;
        c = cv::waitKey(10);
        if ((char)c == 27)
        {
            break;
        }
        if ((char)c == 'u')
        {
            pyrUp(tmp, dst, cv::Size(tmp.cols * 2, tmp.rows * 2));
            printf("** Zoom In:Image x 2\n");
        }
        else if ((char)c == 'd')
        {
            pyrDown(tmp, dst, cv::Size(tmp.cols / 2, tmp.rows / 2));
            printf("**Zoom Out:Image / 2\n");
        }
        imshow(window_name, dst);
        tmp = dst;
    }
}

int main()
{
	const char* path3 = R"(D:\wkp\GScopeStitching\image-20230901\1_0)";
	const int start = 21;
	const int finish = 22;

#if 1
	auto fileType = FileList::ENUM_FILETYPE::FILETYPE_TIFF;
	const char* path = R"(D:\wkp\GScopeStitching\GScopeStitching-mfc\data\HL-10-07-09\CL_0)";
#else
	auto fileType = FileList::ENUM_FILETYPE::FILETYPE_JPG;
	const char* path = R"(D:\wkp\GScopeStitching\GScopeStitching-mfc\data\CL_0)";
#endif

	auto filelist = FileList::NewFileList(fileType, finish, path3);
	Stitching stitch(filelist);
	stitch.SetParameter(start, finish, 'N', 500, "result.tif", 0.4, 0.1);
	stitch.Start();

	//ReadImg();
    //SFImg();
	system("pause");
	return 0;
}
