#include<opencv2/opencv.hpp>
#include<iostream>
#include "Camera/GXcamera.h"
//#include "Camera/Camera.h"
//int a =100;
//uint8_t tx[16] = {0};
//void n () {
//    tx[0] = 0xff;
//    tx[1] = a >> 8;
//    tx[2] = a;
//    write(serial_port,tx,sizieof(tx);
//    read(serial_port,rx,10)
//}
//uint8_t rx[10]={0x55,0xa1,0xff,0xaa,0xff};
//void b()
//{
//
//    if(rx[0] == 0x55)
//    {
//        int sum +=rx[0];
//        int speed = (rx[1]<<8) +rx[2];
//        sum+=rx[1]+rx[2];
//
//        if(sum ==)
//    }
//}

using namespace std;
using namespace cv;
vector<Point2f> armorVertices;
Point2f centerPoint;

class LightDescriptor
{
public:
    LightDescriptor() {}
    LightDescriptor(const cv::RotatedRect& light)
    {
        width = light.size.width;
        length = light.size.height;
        center = light.center;
        if (light.angle > 135.0)
            angle = light.angle - 180.0;
        else
            angle = light.angle;
        area = light.size.area();
    }
    const LightDescriptor& operator =(const LightDescriptor& ld)
    {
        this->width = ld.width;
        this->length = ld.length;
        this->center = ld.center;
        this->angle = ld.angle;
        this->area = ld.area;
        return *this;
    }

    RotatedRect rec() const
    {
        return RotatedRect(center, Size2f(width, length), angle);
    }

public:
    float width;
    float length;
    Point2f center;
    float angle;
    float area;
};

enum Colors
{
    Blue = 0,
    Red = 1
};

int main()
{
    DaHengCamera DaHeng;
    DaHeng.StartDevice(1);
    // 设置分辨率
    DaHeng.SetResolution(1,1);
    //更新时间戳，设置时间戳偏移量
    // 开始采集帧
    DaHeng.SetStreamOn();
    // 设置曝光事件
    DaHeng.SetExposureTime(3500);
    // 设置1
    DaHeng.SetGAIN(3, 16);
    // 是否启用自动白平衡7
    // DaHeng.Set_BALANCE_AUTO(0);
    // manual白平衡 BGR->012
    DaHeng.Set_BALANCE(0,1.56);
    DaHeng.Set_BALANCE(1,1.0);
    DaHeng.Set_BALANCE(2,1.548);
    // // Gamma
    // DaHeng.Set_Gamma(1,1.0);
    // //Color
    // DaHeng.Color_Correct(1);
    // //Contrast
    // DaHeng.Set_Contrast(1,10);
    // //Saturation
    // DaHeng.Set_Saturation(0,0);
    //VideoCapture capture("/home/shaobing2/桌面/5.mp4");

    Mat img;
    int enemyColor = Red;

    while (1) {


        auto time_cap = std::chrono::steady_clock::now();
        DaHeng.UpdateTimestampOffset(time_cap);

        auto DaHeng_stauts = DaHeng.GetMat(img);
        if (!DaHeng_stauts) {
            cout << "src is empty!" << endl;
        }

        // src.timestamp = DaHeng.Get_TIMESTAMP();


        if (img.empty()) {
            cout << "src is empty!" << endl;
            break;
        }

        Mat src;

        img.copyTo(src);
        imshow("111", src);

        vector<Mat> channels;
        split(img, channels);

        if (enemyColor == Red) {
            img = channels[2] - channels[0];
        } else {
            img = channels[0] - channels[2];
        }
        threshold(img, img, 80, 255, THRESH_BINARY);
        Mat element = getStructuringElement(MORPH_ELLIPSE, Size(3, 3));
        dilate(img, img, element);
        vector<vector<Point>> lightContours;
        vector<float> height;
        findContours(img, lightContours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
        vector<LightDescriptor> lightInfos;
        for (const auto &contour: lightContours) {
            // 求轮廓面积
            float area = contourArea(contour);
            if (contour.size() < 5) continue;
            // 用椭圆拟合区域得到外接矩形
            RotatedRect Light_Rec = fitEllipse(contour);
            if (area < 15) continue;
            // 长宽比和轮廓面积比限制
            if (Light_Rec.size.width / Light_Rec.size.height > 5) continue;
            if (area / Light_Rec.size.area() < 0.25) continue;
            if (Light_Rec.angle < 170 && Light_Rec.angle > 10) continue;
            height.push_back(Light_Rec.size.height);
            int maxheight = *max_element(height.begin(), height.end());
            if (Light_Rec.size.height * 1.1 < maxheight) continue;
            lightInfos.emplace_back(Light_Rec);
        }

        sort(lightInfos.begin(), lightInfos.end(), [](const LightDescriptor &ld1, const LightDescriptor &ld2) {
            return ld1.center.x < ld2.center.x;
        });

        for (size_t i = 0; i < lightInfos.size(); i++) {
            for (size_t j = i + 1; (j < lightInfos.size()); j++) {
                LightDescriptor &leftLight = lightInfos[i];
                LightDescriptor &rightLight = lightInfos[j];

                //角差
                float angleDiff_ = abs(leftLight.angle - rightLight.angle);
                //长度差比率
                float LenDiff_ratio =
                        abs(leftLight.length - rightLight.length) / max(leftLight.length, rightLight.length);
                //筛选
                if (angleDiff_ > 5 || LenDiff_ratio > 0.3) continue;
                //左右灯条相距距离
                float dis = pow(pow((leftLight.center.x - rightLight.center.x), 2) +
                                pow((leftLight.center.y - rightLight.center.y), 2), 0.5);
                //左右灯条长度的平均值
                float meanLen = (leftLight.length + rightLight.length);
                //左右灯条长度差比值
                float lendiff = abs(leftLight.length - rightLight.length) / meanLen;
                if (lendiff > 0.04) continue;
                //左右灯条中心点y的差值
                float yDiff = abs(leftLight.center.y - rightLight.center.y);
                //y差比率
                float yDiff_ratio = yDiff / meanLen;
                if (yDiff_ratio > 0.3) continue;
                //左右灯条中心点x的差值
                float xDiff = abs(leftLight.center.x - rightLight.center.x);
                //x差比率
                float xDiff_ratio = xDiff / meanLen;
                if (xDiff_ratio > 2) continue;
                //相距距离与灯条长度比值
                float ratio = dis / meanLen;
                if (ratio > 3) continue;

                Point center = Point((leftLight.center.x + rightLight.center.x) / 2,
                                     (leftLight.center.y + rightLight.center.y) / 2);
                Mat src;
                img.copyTo(src);
                circle(src, center, 3, Scalar(255, 0, 255), FILLED);
                putText(src, to_string(center.x), Point(center.x + 5, center.y), 0.3, 0.5, Scalar(240, 125, 50), 1.5);
                putText(src, to_string(center.y), Point(center.x + 5, center.y + 12), 0.3, 0.5, Scalar(240, 125, 50),
                        1.5);
                RotatedRect rect = RotatedRect(center, Size(dis, meanLen), (leftLight.angle + rightLight.angle) / 2);
                Point2f vertices[4];
                rect.points(vertices);
                for (int i = 0; i < 4; i++) {
                    line(src, vertices[i], vertices[(i + 1) % 4], Scalar(200, 255, 0), 1, LINE_AA);
                }


            }

        }
        imshow("armor", img);
        waitKey(1);
    }
}
