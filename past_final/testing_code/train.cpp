#include <fcntl.h> 
#include <iostream>
#include <fstream>
#include <sstream>
#include <linux/fb.h>
#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <sys/ioctl.h>
#include <string>
#include <thread>
#include <mutex>
#include <termios.h>
#include <unistd.h>
#include <math.h>
#include <vector>

using namespace cv;
using namespace cv::face;
using namespace std;

// create and return normalized image
static Mat norm_0_255(InputArray _src) {    
    Mat src = _src.getMat();          
    Mat dst;    
    switch (src.channels()) {    
        case 1:        
            cv::normalize(_src, dst, 0, 255, NORM_MINMAX, CV_8UC1);        
            break;    
        case 3:       
            cv::normalize(_src, dst, 0, 255, NORM_MINMAX, CV_8UC3);        
            break;    
        default:        
            src.copyTo(dst);        
            break;    
    }   
    return dst; 
}

// read the csv the indicatee the image files in the forder  
static bool read_csv(const string& filename, vector<Mat>& images, vector<int>& labels, char separator = ';') {    
    std::ifstream file(filename.c_str(), ifstream::in);    
    if (!file) {        
        string error_message = "No valid input file was given, please check the given filename.";        
        //CV_Error(CV_StsBadArg, error_message);   
        cout << "CV ERROR: " << error_message << endl;
        return false;
    }    
    
    string line, path, classlabel;    
    while (getline(file, line)) {        
        
        //cout << "line: " << line << endl;
        stringstream liness(line);        
        getline(liness, path, separator);        
        getline(liness, classlabel);
        //cout << "path: " << path << endl;
        //cout << "image: " << imread(path,0) << endl;

        if (!path.empty() && !classlabel.empty()) {            
            images.push_back(imread(path, 0));            
            labels.push_back(atoi(classlabel.c_str()));       
        }  
    }
    return true;
}
int main(){    
    
    // read_csv   
    string fn_csv = "./at.txt";    
   
    vector<Mat> images;    
    vector<int> labels;    
     
    if (read_csv(fn_csv, images, labels) == false) {
        // read csv error: no such file
        exit(1); 
    }       
    
    if (images.size() <= 1) {        
        string error_message = "This demo needs at least 2 images to work. Please add more images to your data set!";        
        //CV_Error(CV_StsError, error_message);   
        cout << "images size: " << images.size() << error_message << endl; 
    }    
    

    // use the last image to test
    const int TEST_SIZE = 1;
    vector<Mat> testSamples;
    vector<int> testLabels;
    for(int i = 1;i <= TEST_SIZE;i++){
        testSamples.push_back(images[images.size()-1]);
        testLabels.push_back(labels[labels.size()-1]);
        images.pop_back();
        labels.pop_back();
    }     
    
    Ptr<FaceRecognizer> model = EigenFaceRecognizer::create();    
    model->train(images, labels);    
    model->save("MyFaceEigenModel.xml");    
    Ptr<FaceRecognizer> model1 = FisherFaceRecognizer::create();    
    model1->train(images, labels);    
    model1->save("MyFaceFisherModel.xml");   
    Ptr<FaceRecognizer> model2 = LBPHFaceRecognizer::create();    
    model2->train(images, labels);    
    model2->save("MyFaceLBPHModel.xml");   
  
    int predictedLabel;
    double confidence;
    int correct = 0;
    int correct1 = 0;
    int correct2  = 0;
    for(int i = 0;i < TEST_SIZE;i++){
        model->predict(testSamples[i],predictedLabel,confidence);
        if(predictedLabel == testLabels[i]){
            correct++;
        }
        model1->predict(testSamples[i],predictedLabel,confidence);
        if(predictedLabel == testLabels[i]){
            correct1++;
        }
        model2->predict(testSamples[i],predictedLabel,confidence);
        if(predictedLabel == testLabels[i]){
            correct2++;
        }
    }
    cout << "Model Eigen accuracy: " << (double)correct<< endl;
    cout << "Model Fisher accuracy: " << (double)correct1 << endl;
    cout << "Model LBPH accuracy: " << (double)correct2 << endl;
    waitKey(0);    
    return 0;
 }   