
#include "OpenCVWrapper.h"
#import "UIImage+OpenCV.h"
#include "VirtualScreen.hpp"
#include <vector>
#include <stdlib.h>
#include <list>

using namespace cv;
using namespace std;


@implementation OpenCVWrapper : NSObject


UIImage *overlapStillUIImage = [UIImage imageNamed:@"textbook.jpg"];
cv::Mat overlapStillImage = [overlapStillUIImage CVMat];
NSString* videoPath=[[NSBundle mainBundle] pathForResource:@"TestVideo" ofType:@"mov"];
VideoCapture capture(videoPath.UTF8String);

cv::Mat lastImage;
int disappearCount;

+ (UIImage *)processImageWithOpenCV:(UIImage*)inputImage {
    
    cv::Mat cameraImage = [inputImage CVMat];
    
    // QRCode requires gray image
    cv::Mat grayImage;
    cv::cvtColor(cameraImage, grayImage, CV_BGR2GRAY);
    
    vector<qrComb> qrQuadPairs = getQRResult(grayImage);
    
    if (! qrQuadPairs.empty()) {
        
        cv::Mat overlayImage = getOverlayImage(overlapStillImage, capture);
        
        cv::Mat perspectiveTransformMatrix = getPerspectiveTransformMatrix(qrQuadPairs);
        
        cameraImage = overlayImageOnCamera(overlayImage, cameraImage, perspectiveTransformMatrix);
        
        disappearCount = 0;
        lastImage = cameraImage;
    } else {
        if (disappearCount++ < 5) {
            cameraImage = lastImage;
        }
    }
    
    return [UIImage imageWithCVMat:cameraImage];
}

@end