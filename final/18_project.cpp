#include <fcntl.h> 
#include <fstream>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

using namespace cv;
using namespace dnn;
using namespace std;

struct framebuffer_info
{
    uint32_t bits_per_pixel;    // framebuffer depth
    uint32_t xres_virtual;      // how many pixel in a row in virtual screen
    uint32_t yres_virtual;      // how many pixel in a column in virtual screen
};

struct framebuffer_info get_framebuffer_info(const char *framebuffer_device_path);

// Initialize the parameters
float confThreshold = 0.5; // Confidence threshold
float nmsThreshold = 0.4;  // Non-maximum suppression threshold
int inpWidth = 416;  // Width of network's input image
int inpHeight = 416; // Height of network's input image
vector<string> classes;

// Remove the bounding boxes with low confidence using non-maxima suppression
void postprocess(Mat& frame, const vector<Mat>& out);

// Draw the predicted bounding box
void drawPred(int classId, float conf, int left, int top, int right, int bottom, Mat& frame);

// Get the names of the output layers
vector<String> getOutputsNames(const Net& net);

int main(int argc, char* argv[])
{

    framebuffer_info fb_info = get_framebuffer_info("/dev/fb0");
    std::ofstream ofs("/dev/fb0");

    int framebuffer_width  = fb_info.xres_virtual;
    int framebuffer_height = fb_info.yres_virtual;
    int framebuffer_depth  = fb_info.bits_per_pixel;
    printf( "xres_virtual : %d\n", fb_info.xres_virtual);
    printf( "yres_virtual : %d\n", fb_info.yres_virtual);
    printf( "bits_per_pixel : %d\n", fb_info.bits_per_pixel);

    // Load names of classes
    string classesFile = "coco.names";
    ifstream ifs(classesFile.c_str());
    string line;
    while (getline(ifs, line)) classes.push_back(line);

    String modelConfiguration;
    String modelWeights;
    
    if (strcmp(argv[1], "basic") == 0) {
        // Give the configuration and weight files for the model
        modelConfiguration = "yolov3.cfg";
        modelWeights = "yolov3.weights";

        cout<<"load model"<<endl;
        // Load the network
        Net net = readNetFromDarknet(modelConfiguration, modelWeights);
        net.setPreferableBackend(DNN_TARGET_CPU);
        
        framebuffer_info fb_info = get_framebuffer_info("/dev/fb0");
        std::ofstream ofs("/dev/fb0");
        cv::Mat framebuffer_BGR565;
        
        // Load image
        cout<<"load image"<<endl;
        Mat frame, blob;
        frame = imread("example001.png");
        string outputFile = "example001_yolo_out_cpp.png";

        // Create a 4D blob from a frame.
        blobFromImage(frame, blob, 1/255.0, cv::Size(inpWidth, inpHeight), Scalar(0,0,0), true, false);
        
        //Sets the input to the network
        net.setInput(blob);
        
        cout<<"inference"<<endl;
        // Runs the forward pass to get output of the output layers
        vector<Mat> outs;
        net.forward(outs, getOutputsNames(net));
        
        // Remove the bounding boxes with low confidence
        postprocess(frame, outs);
        
        // Put efficiency information. The function getPerfProfile returns the overall time for inference(t) and the timings for each of the layers(in layersTimes)
        vector<double> layersTimes;
        double freq = getTickFrequency() / 1000;
        double t = net.getPerfProfile(layersTimes) / freq;
        string label = format("Inference time for a frame : %.2f ms", t);
        putText(frame, label, Point(0, 15), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 255));
        
        // Write the frame with the detection boxes
        Mat detectedFrame;
        frame.convertTo(detectedFrame, CV_8U);
        imwrite(outputFile, detectedFrame);        
        
        cv::Size2f frame_size;
        frame_size = detectedFrame.size();
        cv::cvtColor(detectedFrame, framebuffer_BGR565, cv::COLOR_BGR2BGR565);
        for (int y = 0; y < frame_size.height ; y++) {
            // 2 is because one pixel has two byte (fb_info.bits_per_pixel/8)
            ofs.seekp(y*framebuffer_width*2);
            ofs.write(reinterpret_cast<char*>(framebuffer_BGR565.ptr(y)), frame_size.width*2);
        }

    } else if (strcmp(argv[1], "advanced") == 0) {
        // Give the configuration and weight files for the model
        modelConfiguration = "yolov3-tiny.cfg";
        modelWeights = "yolov3-tiny.weights";

        // Load the network
        Net net = readNetFromDarknet(modelConfiguration, modelWeights);
        net.setPreferableBackend(DNN_TARGET_CPU);

        // setting the parameters of the video stream
        const int frame_width = 1280; 
        const int frame_height = 720;
        const int frame_rate = 30;

        // open video stream device
        // https://docs.opencv.org/3.4.7/d8/dfe/classcv_1_1VideoCapture.html#a5d5f5dacb77bbebdcbfb341e3d4355c1
        cv::VideoCapture cap (2);

        framebuffer_info fb_info = get_framebuffer_info("/dev/fb0");
        std::ofstream ofs("/dev/fb0");
        cv::Mat framebuffer_BGR565;

        // check if video stream device is opened success or not
        // https://docs.opencv.org/3.4.7/d8/dfe/classcv_1_1VideoCapture.html#a9d2ca36789e7fcfe7a7be3b328038585
        if(!cap.isOpened()) {
            printf("Could not open video device\n");
            return 1;
        }

        cap.set(CV_CAP_PROP_FRAME_WIDTH, frame_width); // fb_info.xres_virtual
        cap.set(CV_CAP_PROP_FRAME_HEIGHT, frame_height); // fb_info.yres_virtual
        cap.set(CV_CAP_PROP_FPS, frame_rate);
        cap.set(CV_CAP_PROP_BUFFERSIZE, 1);
        
        // Create a window
        //namedWindow("YOLOv3 ObjectDetection", WINDOW_AUTOSIZE);

        cv::Mat frame, blob;
        cv::Size2f frame_size;
        while (true) {
            cap >> frame;

            // Create a 4D blob from a frame.
            blobFromImage(frame, blob, 1/255.0, cv::Size(inpWidth, inpHeight), Scalar(0,0,0), true, false);
            
            //Sets the input to the network
            net.setInput(blob);
            
            cout<<"inference"<<endl;
            // Runs the forward pass to get output of the output layers
            vector<Mat> outs;
            net.forward(outs, getOutputsNames(net));
            
            // Remove the bounding boxes with low confidence
            postprocess(frame, outs);
            
            // Put efficiency information. The function getPerfProfile returns the overall time for inference(t) and the timings for each of the layers(in layersTimes)
            vector<double> layersTimes;
            double freq = getTickFrequency() / 1000;
            double t = net.getPerfProfile(layersTimes) / freq;
            printf("Inference time for a frame : %.2f ms", t);
            
            // Write the frame with the detection boxes
            Mat detectedFrame;
            frame.convertTo(detectedFrame, CV_8U);

            frame_size = frame.size();

            cv::cvtColor(frame, framebuffer_BGR565, cv::COLOR_BGR2BGR565);
            for (int y = 0; y < frame_size.height ; y++) {
                // 2 is because one pixel has two byte (fb_info.bits_per_pixel/8)
                ofs.seekp(180*framebuffer_width*2 + y*framebuffer_width*2 + 320*2);
                ofs.write(reinterpret_cast<char*>(framebuffer_BGR565.ptr(y)), frame_size.width*2);
            }            
        }
        cap.release();
    }
    
    return 0;
}

// Remove the bounding boxes with low confidence using non-maxima suppression
void postprocess(Mat& frame, const vector<Mat>& outs)
{
    vector<int> classIds;
    vector<float> confidences;
    vector<Rect> boxes;
    
    for (size_t i = 0; i < outs.size(); ++i)
    {
        // Scan through all the bounding boxes output from the network and keep only the
        // ones with high confidence scores. Assign the box's class label as the class
        // with the highest score for the box.
        float* data = (float*)outs[i].data;
        for (int j = 0; j < outs[i].rows; ++j, data += outs[i].cols)
        {
            Mat scores = outs[i].row(j).colRange(5, outs[i].cols);
            Point classIdPoint;
            double confidence;
            // Get the value and location of the maximum score
            minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);
            if (confidence > confThreshold)
            {
                int centerX = (int)(data[0] * frame.cols);
                int centerY = (int)(data[1] * frame.rows);
                int width = (int)(data[2] * frame.cols);
                int height = (int)(data[3] * frame.rows);
                int left = centerX - width / 2;
                int top = centerY - height / 2;
                
                classIds.push_back(classIdPoint.x);
                confidences.push_back((float)confidence);
                boxes.push_back(Rect(left, top, width, height));
            }
        }
    }
    
    // Perform non maximum suppression to eliminate redundant overlapping boxes with
    // lower confidences
    vector<int> indices;
    NMSBoxes(boxes, confidences, confThreshold, nmsThreshold, indices);
    for (size_t i = 0; i < indices.size(); ++i)
    {
        int idx = indices[i];
        Rect box = boxes[idx];
        drawPred(classIds[idx], confidences[idx], box.x, box.y,
                 box.x + box.width, box.y + box.height, frame);
    }
}

// Draw the predicted bounding box
void drawPred(int classId, float conf, int left, int top, int right, int bottom, Mat& frame)
{
    //Draw a rectangle displaying the bounding box
    rectangle(frame, Point(left, top), Point(right, bottom), Scalar(255, 178, 50), 3);
    
    //Get the label for the class name and its confidence
    string label = format("%.2f", conf);
    if (!classes.empty())
    {
        CV_Assert(classId < (int)classes.size());
        label = classes[classId] + ":" + label;
    }
    
    //Display the label at the top of the bounding box
    int baseLine;
    Size labelSize = getTextSize(label, FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);
    top = max(top, labelSize.height);
    rectangle(frame, Point(left, top - round(1.5*labelSize.height)), Point(left + round(1.5*labelSize.width), top + baseLine), Scalar(255, 255, 255), FILLED);
    putText(frame, label, Point(left, top), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(0,0,0),1);
}

// Get the names of the output layers
vector<String> getOutputsNames(const Net& net)
{
    static vector<String> names;
    if (names.empty())
    {
        //Get the indices of the output layers, i.e. the layers with unconnected outputs
        vector<int> outLayers = net.getUnconnectedOutLayers();
        
        //get the names of all the layers in the network
        vector<String> layersNames = net.getLayerNames();
        
        // Get the names of the output layers in names
        names.resize(outLayers.size());
        for (size_t i = 0; i < outLayers.size(); ++i)
        names[i] = layersNames[outLayers[i] - 1];
    }
    return names;
}


struct framebuffer_info get_framebuffer_info(const char* framebuffer_device_path) {

    struct framebuffer_info fb_info;        // Used to return the required attrs.
    struct fb_var_screeninfo screen_info;   // Used to get attributes of the device from OS kernel.

    // open device with linux system call "open()"
    // https://man7.org/linux/man-pages/man2/open.2.html

    // get attributes of the framebuffer device thorugh linux system call "ioctl()".
    // the command you would need is "FBIOGET_VSCREENINFO"
    // https://man7.org/linux/man-pages/man2/ioctl.2.html
    // https://www.kernel.org/doc/Documentation/fb/api.txt

    int fd = -1;
    fd = open(framebuffer_device_path, O_RDWR);
    if (fd >= 0) {
        if (!ioctl(fd, FBIOGET_VSCREENINFO, &screen_info)) {
            // put the required attributes in variable "fb_info" you found with "ioctl() and return it."
            fb_info.xres_virtual = screen_info.xres_virtual;
            fb_info.yres_virtual = screen_info.yres_virtual;
            fb_info.bits_per_pixel = screen_info.bits_per_pixel;
        } 
    } 
    return fb_info;
};