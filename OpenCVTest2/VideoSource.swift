//
//  VideoSource.swift
//  OpenCVTest2
//
//  Created by Pufan Jiang on 7/13/16.
//  Copyright © 2016 Pufan Jiang. All rights reserved.
//

import AVFoundation

struct VideoFrame {
    var width: Int
    var height: Int
    var stride: Int
    var data: UnsafeMutablePointer<Void>
}

protocol VideoSourceDelegate {
    func frameReady(frame: VideoFrame)
}

class VideoSource: NSObject, AVCaptureVideoDataOutputSampleBufferDelegate {
    
    var _captureSession: AVCaptureSession?
    var delegate: VideoSourceDelegate?
    
    override init() {
        super.init()
        
        let captureSession = AVCaptureSession()
        captureSession.sessionPreset = AVCaptureSessionPreset640x480
        // captureSession.sessionPreset = AVCaptureSessionPreset1280x720
        
        _captureSession = captureSession
    }
    
    deinit {
        _captureSession?.stopRunning()
    }
    
    func startDevice() -> Bool {
        do {
            // (1) Find camera device at the specific position
            let backCamera = AVCaptureDevice.defaultDeviceWithMediaType(AVMediaTypeVideo)
            
            // (2) Obtain input port for camera device
            do {
                let input = try AVCaptureDeviceInput(device: backCamera)
                
                // (3) Configure input port for captureSession
                if ((_captureSession?.canAddInput(input)) != nil) {
                    _captureSession?.addInput(input)
                }
                
                // (4) Configure output port for captureSession
                addVideoDataOutput()
                
                // (5) Start captureSession running
                _captureSession?.startRunning()
                
                return true
            } catch {
                print("Debug: error when try AVCaptureDeviceInput.")
            }
            return false
        }
    }
    
    func addVideoDataOutput() {
        // (1) Instantiate a new video data output object
        let captureOutput = AVCaptureVideoDataOutput()
        captureOutput.alwaysDiscardsLateVideoFrames = true
        
        // (2) The sample buffer delegate requires a serial dispatch queue
        let queue = dispatch_queue_create("pufan_queue", DISPATCH_QUEUE_SERIAL)
        captureOutput.setSampleBufferDelegate(self, queue: queue)
        
        // (3) Define the pixel format for the video data output
        let key = kCVPixelBufferPixelFormatTypeKey as NSString
        let value = NSNumber(unsignedInt: kCVPixelFormatType_32BGRA)
        captureOutput.videoSettings = [key : value]
        
        _captureSession?.addOutput(captureOutput)
    }
    
    func captureOutput(captureOutput: AVCaptureOutput!, didOutputSampleBuffer sampleBuffer: CMSampleBuffer!, fromConnection connection: AVCaptureConnection!) {
        // (1) Convert CMSampleBufferRef to CVImageBufferRef
        let imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer)
        
        // (2) Lock pixel buffer
        CVPixelBufferLockBaseAddress(imageBuffer!, kCVPixelBufferLock_ReadOnly)
        
        // (3) Construct VideoFrame struct
        let baseAddress = CVPixelBufferGetBaseAddress(imageBuffer!)
        let width = CVPixelBufferGetWidth(imageBuffer!)
        let height = CVPixelBufferGetHeight(imageBuffer!)
        let stride = CVPixelBufferGetBytesPerRow(imageBuffer!)
        let frame = VideoFrame(width: width, height: height, stride: stride, data: baseAddress)
        
        // (4) Dispatch VideoFrame to VideoSource delegate
        self.delegate!.frameReady(frame)
        
        // (5) Unlock pixel buffer
        CVPixelBufferUnlockBaseAddress(imageBuffer!, 0)
    }
}
