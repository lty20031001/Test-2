//
// Created by shaobing2 on 8/26/23.
//

#ifndef YZW_TEXT_DETECTOR_H
#define YZW_TEXT_DETECTOR_H



#include<opencv2/opencv.hpp>
#include<iostream>



using namespace std;
using namespace cv;


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



#endif //YZW_TEXT_DETECTOR_H
