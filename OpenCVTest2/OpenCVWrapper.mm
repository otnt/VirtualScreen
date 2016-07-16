
#include "OpenCVWrapper.h"
#import "UIImage+OpenCV.h"

#include <stdlib.h>
#include <opencv2/opencv.hpp>
#include "zbar.h"
#include <stdlib.h>
#include <list>

using namespace cv;
using namespace std;


@implementation OpenCVWrapper : NSObject

struct qrComb {
    cv::Point2f inputQuad[4];
    cv::Point2f outputQuad[4];
};

zbar::ImageScanner scanner;
// Input Quadilateral or Image plane coordinates
//cv::Point2f inputQuad[4];
// Output Quadilateral or World plane coordinates
vector<qrComb> outputQuad;
// Which QR code
int whichQR = -1;

UIImage *overlapStillUIImage = [UIImage imageNamed:@"textbook.jpg"];
cv::Mat overlapStillImage = [overlapStillUIImage CVMat];

NSString* videoPath=[[NSBundle mainBundle] pathForResource:@"TestVideo" ofType:@"mov"];
VideoCapture capture(videoPath.UTF8String);

list<cv::Mat> prevImages;

cv::Mat lastImages;
int disappearCount;

cv::Mat smoothedPerspectiveMap;
list<cv::Mat> prevPerspectiveMaps;

+ (UIImage *)processImageWithOpenCV:(UIImage*)inputImage {
    
    cv::Mat mat = [inputImage CVMat];
    cv::Mat grayMatForQRScan;
    cv::Mat smoothedImage = mat;
    
    // QRCode requires gray image
    cv::cvtColor(mat, grayMatForQRScan, CV_BGR2GRAY);
    
    const int ROWS = mat.rows;
    const int COLS = mat.cols;
    
    scanner.set_config(zbar::ZBAR_QRCODE, zbar::ZBAR_CFG_ENABLE, 1);
    
    // wrap image data
    zbar::Image image(COLS, ROWS, "Y800", grayMatForQRScan.ptr(), ROWS * COLS);
    int n = scanner.scan(image);
    
    cout << "found " << n << endl;
    
    if (n > 0) {
        outputQuad.clear();
        
        // extract results
        for(zbar::Image::SymbolIterator symbol = image.symbol_begin();
            symbol != image.symbol_end(); ++symbol) {
            
            whichQR = atoi(symbol->get_data().c_str());
            
            // cout << "decoded " << symbol->get_type_name() << " symbol \"" << symbol->get_data() << "\"" <<" "<< endl;
            int n = symbol->get_location_size();
            qrComb tmpOutputQuad;
            
            switch (whichQR) {
                case 1:
                    tmpOutputQuad.inputQuad[0] = cv::Point2f(166, 2264);
                    tmpOutputQuad.inputQuad[1] = cv::Point2f(166, 3133);
                    tmpOutputQuad.inputQuad[2] = cv::Point2f(1033, 3133);
                    tmpOutputQuad.inputQuad[3] = cv::Point2f(1033, 2264);
                    break;
                case 2:
                    tmpOutputQuad.inputQuad[0] = cv::Point2f(166, 160);
                    tmpOutputQuad.inputQuad[1] = cv::Point2f(166, 1030);
                    tmpOutputQuad.inputQuad[2] = cv::Point2f(1033, 1030);
                    tmpOutputQuad.inputQuad[3] = cv::Point2f(1033, 160);
                    break;
                case 3:
                    tmpOutputQuad.inputQuad[0] = cv::Point2f(1232, 160);
                    tmpOutputQuad.inputQuad[1] = cv::Point2f(1232, 1030);
                    tmpOutputQuad.inputQuad[2] = cv::Point2f(2105, 1030);
                    tmpOutputQuad.inputQuad[3] = cv::Point2f(2105, 160);
                    break;
                case 4:
                    tmpOutputQuad.inputQuad[0] = cv::Point2f(166, 1228);
                    tmpOutputQuad.inputQuad[1] = cv::Point2f(166, 2081);
                    tmpOutputQuad.inputQuad[2] = cv::Point2f(1033, 2081);
                    tmpOutputQuad.inputQuad[3] = cv::Point2f(1033, 1228);
                    break;
                case 5:
                    tmpOutputQuad.inputQuad[0] = cv::Point2f(1232, 1228);
                    tmpOutputQuad.inputQuad[1] = cv::Point2f(1232, 2081);
                    tmpOutputQuad.inputQuad[2] = cv::Point2f(2105, 2081);
                    tmpOutputQuad.inputQuad[3] = cv::Point2f(2105, 1228);
                    break;
                default:
                    cerr << "Unknown QR Code" << whichQR << endl;
                    break;
            }
            
            for(int i = 0; i < n; i++){
                tmpOutputQuad.outputQuad[i].x = symbol->get_location_x(i);
                tmpOutputQuad.outputQuad[i].y = symbol->get_location_y(i);
            }
            outputQuad.push_back(tmpOutputQuad);
        }
        
        // Put video onto overlap image
        cv::Mat videoFrame;
        capture >> videoFrame;
        if (videoFrame.empty()) {
            // Reload
            capture.open(videoPath.UTF8String);
            capture >> videoFrame;
        }
        
        // construct the image we are gonna render at this frame.
        cv::Mat overlapImage = cv::Mat(overlapStillImage);
        for(int row = 0; row < videoFrame.rows && row < overlapImage.rows; ++row) {
            uchar* rowPtr = videoFrame.ptr(row);
            uchar* imageRowPtr = overlapImage.ptr(row);
            for (int col = 0; col < videoFrame.cols && col < overlapImage.cols; ++col) {
                imageRowPtr[4 * col + 0] = rowPtr[3 * col + 0];
                imageRowPtr[4 * col + 1] = rowPtr[3 * col + 1];
                imageRowPtr[4 * col + 2] = rowPtr[3 * col + 2];
            }
        }
        
        
        // Build perspective map.
        vector<cv::Mat> persectiveMaps;
        for (auto iterator = outputQuad.cbegin(); iterator != outputQuad.cend(); ++iterator) {
            auto quad = *iterator;
            persectiveMaps.push_back(cv::getPerspectiveTransform(quad.inputQuad, quad.outputQuad));
        }
        
        cv::Mat perspectiveMap = cv::Mat::zeros(3, 3, CV_64FC1);
        for (auto map = persectiveMaps.cbegin(); map != persectiveMaps.cend(); ++map) {
            double* perspectiveMapData = (double*)perspectiveMap.data;
            double* mapData = (double*)(*map).data;
            for (int i = 0; i < 3 * 3; ++i) {
                *perspectiveMapData += *mapData;
                ++perspectiveMapData;
                ++mapData;
            }
        }
        double* perspectiveMapData = (double*)perspectiveMap.data;
        for (int i = 0; i < 3 * 3; ++i) {
            *perspectiveMapData /= persectiveMaps.size();
            ++perspectiveMapData;
        }
        
        // Smooth perspective map.
        uint gaussian[4] = {867, 8815, 34971, 55347};
        smoothedPerspectiveMap = cv::Mat::zeros(3, 3, CV_64FC1);
        if (prevPerspectiveMaps.size() == 4) {
            prevPerspectiveMaps.front().release();
            prevPerspectiveMaps.pop_front();
        }
        prevPerspectiveMaps.push_back(perspectiveMap);
        size_t perpectiveMapSize = prevPerspectiveMaps.size();
        
        
        double* datas[perpectiveMapSize];
        size_t i = 0;
        for (auto iterator = prevPerspectiveMaps.cbegin(); iterator != prevPerspectiveMaps.cend(); ++iterator) {
            datas[i++] = (double*)(*iterator).data;
        }
        
        double* smoothedPerspectiveMapData = (double*)smoothedPerspectiveMap.data;
        uint normalization = 0;
        for (size_t p = 0; p < perpectiveMapSize; ++p) {
            double* data = datas[p];
            uint gaussianValue = gaussian[4 - perpectiveMapSize + p];
            normalization += gaussianValue;
            double* smoothedPerspectiveMapData = (double*)smoothedPerspectiveMap.data;
            for (int i = 0; i < 3 * 3; ++i) {
                *smoothedPerspectiveMapData += gaussianValue * (*data);
                ++smoothedPerspectiveMapData;
                ++data;
            }
        }
        smoothedPerspectiveMapData = (double*)smoothedPerspectiveMap.data;
        for (int i = 0; i < 3 * 3; ++i) {
            *smoothedPerspectiveMapData /= normalization;
            ++smoothedPerspectiveMapData;
        }
        
        cv::Mat warpedOverlap;
        warpPerspective(overlapImage, warpedOverlap, perspectiveMap, mat.size());
        
        vector<Point2f> boundary;
        vector<Point2f> newBoundary;
        boundary.push_back(Point2f(0, 0));
        boundary.push_back(Point2f(0, 3300));
        boundary.push_back(Point2f(2550, 3300));
        boundary.push_back(Point2f(2550, 0));
        perspectiveTransform(boundary, newBoundary, perspectiveMap);
        
        uchar* matData = mat.data;
        uchar* warpedData = warpedOverlap.data;
        for(int row = 0; row < warpedOverlap.rows; ++row) {
            // uchar* rowPtr = warpedOverlap.ptr(row);
            for (int col = 0; col < warpedOverlap.cols; ++col) {
                cv::Point2f testPoint = cv::Point2f(col, row);
                if (pointPolygonTest(newBoundary, testPoint, false) == 1) {
                    *matData++ = *warpedData++;
                    *matData++ = *warpedData++;
                    *matData++ = *warpedData++;
                    ++matData;
                    ++warpedData;
                } else {
                    matData += 4;
                    warpedData += 4;
                }
            }
        }
        
        disappearCount = 0;
        lastImages = mat;
    } else {
        if (disappearCount++ < 5) {
            mat = lastImages;
        }
    }
    
    return [UIImage imageWithCVMat:mat];
}

@end