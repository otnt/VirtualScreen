//
//  VirtualScreen.cpp
//  OpenCVTest2
//
//  Created by Pufan Jiang on 7/15/16.
//  Copyright © 2016 Pufan Jiang. All rights reserved.
//

#include "VirtualScreen.hpp"
#include <iostream>
#include <list>
#include <opencv2/stitching/detail/warpers.hpp>

using namespace std;

zbar::ImageScanner scanner;

vector<qrComb> getQRResult(cv::Mat grayImage) {
    // Scan image and get result.
    const int ROWS = grayImage.rows;
    const int COLS = grayImage.cols;
    scanner.set_config(zbar::ZBAR_QRCODE, zbar::ZBAR_CFG_ENABLE, 1);
    zbar::Image image(COLS, ROWS, "Y800", grayImage.data, ROWS * COLS);
    int n = scanner.scan(image);
    
    // Return vector.
    vector<qrComb> outputQuad;
    
    cout << "found " << n << endl;
    if (n > 0) {
        // extract results
        for(zbar::Image::SymbolIterator symbol = image.symbol_begin();
            symbol != image.symbol_end(); ++symbol) {
            
            // Each QR Code has a number as data.
            int whichQR = atoi(symbol->get_data().c_str());
            
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
    }
    return outputQuad;
}



cv::Mat getOverlayImage(cv::Mat overlayStillImage, cv::VideoCapture capture) {
    // Put video onto overlap image
    cv::Mat videoFrame;
    capture >> videoFrame;
    if (videoFrame.empty()) {
        // TODO: Reload
        // capture.open(videoPath.UTF8String);
        // capture >> videoFrame;
    }
    
    // construct the image we are gonna render at this frame.
    cv::Mat overlayImage = cv::Mat(overlayStillImage);
    for(int row = 0; row < videoFrame.rows && row < overlayImage.rows; ++row) {
        uchar* rowPtr = videoFrame.ptr(row);
        uchar* imageRowPtr = overlayImage.ptr(row);
        for (int col = 0; col < videoFrame.cols && col < overlayImage.cols; ++col) {
            imageRowPtr[4 * col + 0] = rowPtr[3 * col + 0];
            imageRowPtr[4 * col + 1] = rowPtr[3 * col + 1];
            imageRowPtr[4 * col + 2] = rowPtr[3 * col + 2];
        }
    }
    
    return overlayImage;
}

list<cv::Mat> prevPersectiveTransformMatrixs;
cv::Mat getPerspectiveTransformMatrix(vector<qrComb> outputQuad) {
    // Build perspective transform matrix for each quad pair.
    vector<cv::Mat> persectiveTransformMatrixs;
    for (auto iterator = outputQuad.cbegin(); iterator != outputQuad.cend(); ++iterator) {
        auto quad = *iterator;
        persectiveTransformMatrixs.push_back(cv::getPerspectiveTransform(quad.inputQuad, quad.outputQuad));
    }
    
    // Take mean on all these perspective transform matrix.
    cv::Mat perspectiveTransformMatrix = cv::Mat::zeros(3, 3, CV_64FC1);
    for (auto map = persectiveTransformMatrixs.cbegin(); map != persectiveTransformMatrixs.cend(); ++map) {
        double* perspectiveTransformMatrixData = (double*)perspectiveTransformMatrix.data;
        double* mapData = (double*)(*map).data;
        for (int i = 0; i < 3 * 3; ++i) {
            *perspectiveTransformMatrixData += *mapData;
            ++perspectiveTransformMatrixData;
            ++mapData;
        }
    }
    double* perspectiveTransformMatrixData = (double*)perspectiveTransformMatrix.data;
    for (int i = 0; i < 3 * 3; ++i) {
        *perspectiveTransformMatrixData /= persectiveTransformMatrixs.size();
        ++perspectiveTransformMatrixData;
    }
    
    // Smooth perspective map based on a simple window filter.
    int SMOOTH_FRAME_NUM = 4;
    uint gaussian[4] = {867, 8815, 34971, 55347};
    cv::Mat smoothedPerspectiveTransformMatrix = cv::Mat::zeros(3, 3, CV_64FC1);
    if (prevPersectiveTransformMatrixs.size() == SMOOTH_FRAME_NUM) {
        prevPersectiveTransformMatrixs.front().release();
        prevPersectiveTransformMatrixs.pop_front();
    }
    prevPersectiveTransformMatrixs.push_back(perspectiveTransformMatrix);
    size_t perpectiveTransformMatrixCount = prevPersectiveTransformMatrixs.size();
    
    // Put data pointer into an array, to ease further process.
    double* datas[perpectiveTransformMatrixCount];
    size_t i = 0;
    for (auto iterator = prevPersectiveTransformMatrixs.cbegin(); iterator != prevPersectiveTransformMatrixs.cend(); ++iterator) {
        datas[i++] = (double*)(*iterator).data;
    }
    
    // Window filter using gaussian blur.
    uint normalization = 0;
    for (size_t p = 0; p < perpectiveTransformMatrixCount; ++p) {
        double* data = datas[p];
        uint gaussianValue = gaussian[SMOOTH_FRAME_NUM - perpectiveTransformMatrixCount + p];
        normalization += gaussianValue;
        double* smoothedPerspectiveTransformMatrixData = (double*)smoothedPerspectiveTransformMatrix.data;
        for (int i = 0; i < 3 * 3; ++i) {
            *smoothedPerspectiveTransformMatrixData += gaussianValue * (*data);
            ++smoothedPerspectiveTransformMatrixData;
            ++data;
        }
    }
    double* smoothedPerspectiveTransformMatrixData = (double*)smoothedPerspectiveTransformMatrix.data;
    for (int i = 0; i < 3 * 3; ++i) {
        *smoothedPerspectiveTransformMatrixData /= normalization;
        ++smoothedPerspectiveTransformMatrixData;
    }
    
    return smoothedPerspectiveTransformMatrix;
}

cv::Mat overlayImageOnCamera(cv::Mat overlayImage, cv::Mat cameraImage, cv::Mat perspectiveTransformMatrix) {
    
    // TOO SLOW:
    /*
     uchar* overlayImageData = overlayImage.data;
     for (int row = 0; row < overlayImage.rows; ++row) {
     for (int col = 0; col < overlayImage.cols; ++col) {
     vector<cv::Point2i> oldPosition;
     vector<cv::Point2i> newPosition;
     oldPosition.push_back(cv::Point2i(col, row));
     perspectiveTransform(oldPosition, newPosition, perspectiveTransformMatrix);
     
     cv::Point2i newPoint = cv::Point2i(row, col); //newPosition.front();
     auto r = newPoint.y;
     auto c = newPoint.x;
     if (0 <= c && c < cameraImage.cols
     && 0 <= r && r < cameraImage.rows) {
     cameraImage.ptr(r)[4 * c + 0] = *overlayImageData++;
     cameraImage.ptr(r)[4 * c + 1] = *overlayImageData++;
     cameraImage.ptr(r)[4 * c + 2] = *overlayImageData++;
     ++overlayImageData;
     } else {
     overlayImageData += 4;
     }
     }
     }
     */
    
    cv::Mat warpedOverlayImage;
    warpPerspective(overlayImage, warpedOverlayImage, perspectiveTransformMatrix, cameraImage.size());
    
    vector<cv::Point2f> boundary;
    vector<cv::Point2f> newBoundary;
    boundary.push_back(cv::Point2f(0, 0));
    boundary.push_back(cv::Point2f(0, 3300));
    boundary.push_back(cv::Point2f(2550, 3300));
    boundary.push_back(cv::Point2f(2550, 0));
    perspectiveTransform(boundary, newBoundary, perspectiveTransformMatrix);
    
    uchar* cameraImageData = cameraImage.data;
    uchar* warpedOverlayImageData = warpedOverlayImage.data;
    for(int row = 0; row < warpedOverlayImage.rows; ++row) {
        for (int col = 0; col < warpedOverlayImage.cols; ++col) {
            cv::Point2f testPoint = cv::Point2f(col, row);
            if (pointPolygonTest(newBoundary, testPoint, false) == 1) {
                *cameraImageData++ = *warpedOverlayImageData++;
                *cameraImageData++ = *warpedOverlayImageData++;
                *cameraImageData++ = *warpedOverlayImageData++;
                ++cameraImageData;
                ++warpedOverlayImageData;
            } else {
                cameraImageData += 4;
                warpedOverlayImageData += 4;
            }
        }
    }
    
    printf("%d, %d\n", cameraImage.rows, cameraImage.cols);
    return cameraImage;
}

// Copy-pasted from SO:
// http://stackoverflow.com/questions/1927145/how-to-simulate-fisheye-lens-effect-by-opencv
void sampleImage(const IplImage* arr, float idx0, float idx1, CvScalar& res)
{
    if(idx0<0 || idx1<0 || idx0>(cvGetSize(arr).height-1) || idx1>(cvGetSize(arr).width-1)){
        res.val[0]=0;
        res.val[1]=0;
        res.val[2]=0;
        res.val[3]=0;
        return;
    }
    float idx0_fl=floor(idx0);
    float idx0_cl=ceil(idx0);
    float idx1_fl=floor(idx1);
    float idx1_cl=ceil(idx1);
    
    CvScalar s1=cvGet2D(arr,(int)idx0_fl,(int)idx1_fl);
    CvScalar s2=cvGet2D(arr,(int)idx0_fl,(int)idx1_cl);
    CvScalar s3=cvGet2D(arr,(int)idx0_cl,(int)idx1_cl);
    CvScalar s4=cvGet2D(arr,(int)idx0_cl,(int)idx1_fl);
    float x = idx0 - idx0_fl;
    float y = idx1 - idx1_fl;
    res.val[0]= s1.val[0]*(1-x)*(1-y) + s2.val[0]*(1-x)*y + s3.val[0]*x*y + s4.val[0]*x*(1-y);
    res.val[1]= s1.val[1]*(1-x)*(1-y) + s2.val[1]*(1-x)*y + s3.val[1]*x*y + s4.val[1]*x*(1-y);
    res.val[2]= s1.val[2]*(1-x)*(1-y) + s2.val[2]*(1-x)*y + s3.val[2]*x*y + s4.val[2]*x*(1-y);
    res.val[3]= s1.val[3]*(1-x)*(1-y) + s2.val[3]*(1-x)*y + s3.val[3]*x*y + s4.val[3]*x*(1-y);
}

float xscale;
float yscale;
float xshift;
float yshift;

float getRadialX(float x,float y,float cx,float cy,float k){
    x = (x*xscale+xshift);
    y = (y*yscale+yshift);
    float res = x+((x-cx)*k*((x-cx)*(x-cx)+(y-cy)*(y-cy)));
    return res;
}

float getRadialY(float x,float y,float cx,float cy,float k){
    x = (x*xscale+xshift);
    y = (y*yscale+yshift);
    float res = y+((y-cy)*k*((x-cx)*(x-cx)+(y-cy)*(y-cy)));
    return res;
}

float thresh = 1;
float calc_shift(float x1,float x2,float cx,float k){
    float x3 = x1+(x2-x1)*0.5;
    float res1 = x1+((x1-cx)*k*((x1-cx)*(x1-cx)));
    float res3 = x3+((x3-cx)*k*((x3-cx)*(x3-cx)));
    
    //  std::cerr<<"x1: "<<x1<<" - "<<res1<<" x3: "<<x3<<" - "<<res3<<std::endl;
    
    if(res1>-thresh and res1 < thresh)
        return x1;
    if(res3<0){
        return calc_shift(x3,x2,cx,k);
    }
    else{
        return calc_shift(x1,x3,cx,k);
    }
}

cv::Mat sphericalWarp(cv::Mat cameraInput, float centerX, float centerY, float K = 0.001)
{
    IplImage* src = new IplImage(cameraInput);
    IplImage* dst = cvCreateImage(cvGetSize(src),src->depth,src->nChannels);
    int width = cvGetSize(src).width;
    int height = cvGetSize(src).height;
    
    xshift = calc_shift(0,centerX-1,centerX,K);
    float newcenterX = width-centerX;
    float xshift_2 = calc_shift(0,newcenterX-1,newcenterX,K);
    
    yshift = calc_shift(0,centerY-1,centerY,K);
    float newcenterY = height-centerY;
    float yshift_2 = calc_shift(0,newcenterY-1,newcenterY,K);
    //  scale = (centerX-xshift)/centerX;
    xscale = (width-xshift-xshift_2)/width;
    yscale = (height-yshift-yshift_2)/height;
    
    std::cerr<<xshift<<" "<<yshift<<" "<<xscale<<" "<<yscale<<std::endl;
    std::cerr<<cvGetSize(src).height<<std::endl;
    std::cerr<<cvGetSize(src).width<<std::endl;
    
    for(int j=0;j<cvGetSize(dst).height;j++){
        for(int i=0;i<cvGetSize(dst).width;i++){
            CvScalar s;
            float x = getRadialX((float)i,(float)j,centerX,centerY,K);
            float y = getRadialY((float)i,(float)j,centerX,centerY,K);
            sampleImage(src,y,x,s);
            cvSet2D(dst,j,i,s);
        }
    }
    
    return cv::Mat(dst);
}

cv::Mat splitScreen(cv::Mat cameraImage) {
    cameraImage = sphericalWarp(cameraImage, cameraImage.rows / 2, cameraImage.cols / 2, 0.00001);
    
    const int rows = 480;
    const int cols = 640;
    cv::Mat newCameraImage(rows, cols, cameraImage.type());
    cv::resize(cameraImage, cameraImage, cv::Size(cols / 2, rows));
    
    uchar* cameraImageData = cameraImage.data;
    for (int r = 0; r < rows; ++r) {
        std::copy(cameraImageData, cameraImageData + (cols / 2) * cameraImage.channels(), newCameraImage.ptr(r));
        std::copy(cameraImageData, cameraImageData + (cols / 2) * cameraImage.channels(), newCameraImage.ptr(r) + (cols / 2) * cameraImage.channels());
        cameraImageData += cameraImage.cols * cameraImage.channels();
    }
    
    return newCameraImage;
}








