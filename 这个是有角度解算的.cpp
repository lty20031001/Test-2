#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>
using namespace cv;
using namespace std;
const int ANGLE_TO_UP = 1;
const int WIDTH_GREATER_THAN_HEIGHT = 0;
#define DELAT_MAX 30 //定义限幅滤波误差最大值
typedef int64 filter_; //定义限幅滤波数据类型
filter_ filter(filter_ effective_value, filter_ new_value, filter_ delat_max);
filter_ filter(filter_ effective_value, filter_ new_value, filter_ delat_max)
{
    if ((new_value - effective_value > delat_max) || (effective_value - new_value > delat_max))
    {
        new_value = effective_value;
        return effective_value;
    }
    else
    {
        new_value = effective_value;
        return new_value;
    }
}
// 预处理
RotatedRect& adjustRec(cv::RotatedRect& rec, const int mode)
{
    using std::swap;

    float& width = rec.size.width;
    float& height = rec.size.height;
    float& angle = rec.angle;

    if (mode == WIDTH_GREATER_THAN_HEIGHT)
    {
        if (width < height)
        {
            swap(width, height);
            angle += 90.0;
        }
    }
    while (angle >= 90.0)
        angle -= 180.0;
    while (angle < -90.0)
        angle += 180.0;

    if (mode == ANGLE_TO_UP)
    {
        if (angle >= 45.0)
        {
            swap(width, height);
            angle -= 90.0;
        }
        else if (angle < -45.0)
        {
            swap(width, height);
            angle += 90.0;
        }
    }
    return rec;
} //保持灯条是竖着的
int main(int argc, char* argv[])
{
    // 导入视频
    cv::VideoCapture cap("5.mp4");
    if (!cap.isOpened())
    {
        cout << "警告" << std::endl;
        return -1;
    }
    // 创建窗口
    cv::namedWindow("Armor", cv::WINDOW_NORMAL);
    cv::resizeWindow("Armor", 800, 600);

    int fps = 30;
    int width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    int height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    cv::VideoWriter writer("path", cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, cv::Size(width, height));
    
    cv::Mat frame;
    while (cap.read(frame))
    {
        // 转HSV图像
        cv::Mat imgHSV;
        vector<cv::Mat> hsvSplit;
        cvtColor(frame, imgHSV, COLOR_BGR2HSV);
        split(imgHSV, hsvSplit);
        equalizeHist(hsvSplit[2], hsvSplit[2]);
        merge(hsvSplit, imgHSV);
        // 图像二值化处理
        cv::Mat thresHold;
        threshold(hsvSplit[2], thresHold, 240, 245, THRESH_BINARY);
        // 模糊/膨胀处理
        blur(thresHold, thresHold, Size(3, 3));
        Mat element = getStructuringElement(MORPH_ELLIPSE, Size(3, 3)); // 膨胀
        dilate(thresHold, element, element);
        // 开始寻找轮廓
        vector<RotatedRect> vc;
        vector<RotatedRect> vRec;
        vector<vector<Point>> Light_Contour;
        findContours(element.clone(), Light_Contour, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        // 从面积上选轮廓
        for (unsigned int i = 0; i < Light_Contour.size(); i++)
        {
            // 求轮廓面积
            double Light_Contour_Area = contourArea(Light_Contour[i]);
            // 去除较小轮廓fitllipse的限制条件
            if (Light_Contour_Area < 15 || Light_Contour[i].size() <= 10)
                continue;
            RotatedRect Light_Rec = fitEllipse(Light_Contour[i]);
            Light_Rec = adjustRec(Light_Rec, ANGLE_TO_UP);

            if (Light_Rec.angle > 20)
                continue;
            if (Light_Rec.size.width / Light_Rec.size.height > 3 ||
                Light_Contour_Area / Light_Rec.size.area() < 0.3)
                continue;
            Light_Rec.size.height *= 1.1;
            Light_Rec.size.width *= 1.1;
            vc.push_back(Light_Rec);
        }
        // 从灯条长宽比上来筛选轮廓
        for (size_t i = 0; i < vc.size(); i++)
        {
            for (size_t j = i + 1; (j < vc.size()); j++)
            {
                // 判断是否为相同灯条
                float Contour_angle = abs(vc[i].angle - vc[j].angle); // 角度差
                if (Contour_angle >= 30)
                    continue;
                //长度差比率
                float Contour_Len1 = abs(vc[i].size.height - vc[j].size.height) / max(vc[i].size.height, vc[j].size.height);
                //宽度差比率
                float Contour_Len2 = abs(vc[i].size.width - vc[j].size.width) / max(vc[i].size.width, vc[j].size.width);
                if (Contour_Len1 > 0.5 || Contour_Len2 > 0.5)
                    continue;
                /////////装甲板识别//////
                RotatedRect ARMOR;
                ARMOR.center.x = (vc[i].center.x + vc[j].center.x) / 2.; //中点x坐标
                ARMOR.center.y = (vc[i].center.y + vc[j].center.y) / 2.; //中点y坐标
                ARMOR.angle = (vc[i].angle + vc[j].angle) / 2.;         //角度
                float h, w, yRatio, xRatio;
                h = (vc[i].size.height + vc[j].size.height) / 2; //高度
                // 宽度
                w = sqrt((vc[i].center.x - vc[j].center.x) * (vc[i].center.x - vc[j].center.x) +
                    (vc[i].center.y - vc[j].center.y) * (vc[i].center.y - vc[j].center.y));
                float ratio = w / h; //匹配到的装甲板的长宽比
                xRatio = abs(vc[i].center.x - vc[j].center.x) / h;
                yRatio = abs(vc[i].center.y - vc[j].center.y) / h; //比率
                if (ratio < 1.0 || ratio > 5.0 || xRatio < 0.5 || yRatio > 2.0)
                    continue;
                ARMOR.size.height = h;
                ARMOR.size.width = w;
                vRec.push_back(ARMOR);
                Point2f point1;
                Point2f point2;
                point1.x = vc[i].center.x;
                point1.y = vc[i].center.y + 20;
                point2.x = vc[j].center.x;
                point2.y = vc[j].center.y - 20;
                int xmidnum = (point1.x + point2.x) / 2;
                int ymidnum = (point1.y + point2.y) / 2;
                // 轮廓筛选后处理
                ARMOR.center.x = filter(ARMOR.center.x, xmidnum, DELAT_MAX);
                ARMOR.center.y = filter(ARMOR.center.y, ymidnum, DELAT_MAX);
                Scalar color(100, 100, 55);
                rectangle(frame, point1, point2, color, 2); //将装甲板框起来
                circle(frame, ARMOR.center, 10, color);    //瞄准装甲板中心  击打数字
                /////////////////////角度解算///////////////////////////
                int WIDHT_OF_ARMOR = 135;//长
                int HEIGHT_OF_ARMOR = 135;//高
                Mat cameraMatrix = (Mat_<double>(3, 3) << 2352.005855260262, 0.71022826456503, 720.9540232123025,
                    0, 2359.0592645034560, 563.80455606615621,
                    0.0,
                    0.0,
                    1.0);
                Mat distCoeffs = (Mat_<double>(5, 1) << -0.1221586254151815, 1.12856462559656, 0.0003100056562528, 0.00295456623054622561, -9.275629266306006);
                vector<Point3d> point3d = {
                Point3d(WIDHT_OF_ARMOR / 2.0,HEIGHT_OF_ARMOR / 2.0,0),
                Point3d(WIDHT_OF_ARMOR / 2.0,-HEIGHT_OF_ARMOR / 2.0,0),
                Point3d(-WIDHT_OF_ARMOR / 2.0,-HEIGHT_OF_ARMOR / 2.0,0),
                Point3d(-WIDHT_OF_ARMOR / 2.0,HEIGHT_OF_ARMOR / 2.0,0)
                };//表示3D空间中的四个点的坐标。每个点的坐标通过将 WIDHT_OF_ARMOR 和 HEIGHT_OF_ARMOR 分别除以2得到
                Point2f point3;
                Point2f point4;
                point3.x = vc[i].center.x;
                point3.y = vc[i].center.y - 20;
                point4.x = vc[j].center.x;
                point4.y = vc[j].center.y + 20;
                vector<Point2f> points = {
                 point1,
                 point3,
                 point2,
                 point4
                };
                Mat rvec, tvec;
                solvePnP(point3d, points, cameraMatrix, distCoeffs, rvec, tvec);
                //对距离进行解算
                float dist = sqrt(tvec.at<double>(0) * tvec.at<double>(0) +
                    tvec.at<double>(1) * tvec.at<double>(1) +
                    tvec.at<double>(2) * tvec.at<double>(2)); //可以得到目标与相机之间的距离信息
                //偏航角解算
                float yaw = atan2(tvec.at<double>(0), tvec.at<double>(2));
                //俯仰角解算
                float pitch = atan2(tvec.at<double>(1), tvec.at<double>(2));
                std::cout << "Distance: " << dist << std::endl;
                std::cout << "Yaw: " << yaw << std::endl;
                std::cout << "Pitch: " << pitch << std::endl;
            }
        }
        writer.write(frame);
        imshow("Armor", frame);
        if (waitKey(10) == 27) // 按下 ESC 键退出循环
            break;
    }
    writer.release();
    return 0;
}


