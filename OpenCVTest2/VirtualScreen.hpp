//
//  VirtualScreen.hpp
//  OpenCVTest2
//
//  Created by Pufan Jiang on 7/15/16.
//  Copyright © 2016 Pufan Jiang. All rights reserved.
//

#ifndef VirtualScreen_hpp
#define VirtualScreen_hpp

#include <stdio.h>
#include <opencv2/opencv.hpp>
#include "zbar.h"
#include <vector>

using namespace std;

struct qrComb {
    cv::Point2f inputQuad[4];
    cv::Point2f outputQuad[4];
};

vector<qrComb> getQRResult(cv::Mat grayImage);

cv::Mat getOverlayImage(cv::Mat overlapStillImage, cv::VideoCapture capture);

cv::Mat getPerspectiveTransformMatrix(vector<qrComb> outputQuad);

cv::Mat overlayImageOnCamera(cv::Mat overlayImage, cv::Mat cameraImage, cv::Mat perspectiveTransformMatrix);

cv::Mat testProjection(cv::Mat);

cv::Mat resizeTo640x360(cv::Mat);

cv::Mat splitScreen(cv::Mat);

#endif /* VirtualScreen_hpp */
