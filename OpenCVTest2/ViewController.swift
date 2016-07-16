//
//  ViewController.swift
//  OpenCVTest2
//
//  Created by Pufan Jiang on 7/12/16.
//  Copyright © 2016 Pufan Jiang. All rights reserved.
//

import UIKit
import AVFoundation

class ViewController: UIViewController, UIImagePickerControllerDelegate, UINavigationControllerDelegate, VideoSourceDelegate {
    
    @IBOutlet weak var CameraView: UIImageView!
    
    var videoSource: VideoSource!
    
    override func viewDidLoad() {
        super.viewDidLoad()
        // Do any additional setup after loading the view, typically from a nib.
        
        // Configure Video Source
        videoSource = VideoSource()
        videoSource.delegate = self
        videoSource.startDevice()
    }
    
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
    
    override func viewDidAppear(animated: Bool) {
        super.viewDidAppear(animated)
        
    }
    
    override func viewWillAppear(animated: Bool) {
        super.viewWillAppear(animated)
    }
    
    func frameReady(frame: VideoFrame) {
        dispatch_sync( dispatch_get_main_queue(), {
            // Construct CGContextRef from VideoFrame
            let colorSpace = CGColorSpaceCreateDeviceRGB()
            let newContext = CGBitmapContextCreate(
                frame.data,
                frame.width,
                frame.height,
                8,
                frame.stride,
                colorSpace,
                CGBitmapInfo.ByteOrder32Little.rawValue | CGImageAlphaInfo.PremultipliedFirst.rawValue)
                        
            // Construct CGImageRef from CGContextRef
            let newImage = CGBitmapContextCreateImage(newContext);
            
            // Construct UIImage from CGImageRef
            let image = UIImage(CGImage: newImage!)
            
            let processedImage = OpenCVWrapper.processImageWithOpenCV(image)
            
            // self.CameraView.image = processedImage
            self.CameraView.image = processedImage
        })
    }
}

