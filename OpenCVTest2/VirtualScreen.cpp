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

    return cameraImage;
}


