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
#include <time.h>
#include <vector>

using namespace cv;
using namespace cv::face;
using namespace std;

#define embedded 1

struct framebuffer_info
{
    uint32_t bits_per_pixel;    // framebuffer depth
    uint32_t xres_virtual;      // how many pixel in a row in virtual screen
    uint32_t yres_virtual;      // how many pixel in a column in virtual screen
};

struct framebuffer_info get_framebuffer_info(const char *framebuffer_device_path);

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

int main()
{
    // setting the parameters of image
    const double scale = 3.0;
    const int img_width = 138;
    const int img_height = 168;

    // setting the parameters of the video stream
    const int frame_width = 640; 
    const int frame_height = 480;
    const int frame_rate = 10;

    // setting the parameter of haarcascades
    int minNeighbors = 3;
    Size faceSize(50, 50);

    // video capture settings
    #if embedded
    VideoCapture cap(2);
    #else
    VideoCapture cap(0);
    #endif
    //cap.set(CV_CAP_PROP_FRAME_WIDTH, frame_width); // fb_info.xres_virtual
    //cap.set(CV_CAP_PROP_FRAME_HEIGHT, frame_height); // fb_info.yres_virtual
    //cap.set(CV_CAP_PROP_FPS, frame_rate); 

    framebuffer_info fb_info = get_framebuffer_info("/dev/fb0");
    std::ofstream ofs("/dev/fb0");
    
    // check if video stream device is opened success or not
    // https://docs.opencv.org/3.4.7/d8/dfe/classcv_1_1VideoCapture.html#a9d2ca36789e7fcfe7a7be3b328038585
    if(!cap.isOpened()) {
        printf("Could not open video device\n");
        return 1;
    }

    int framebuffer_width  = fb_info.xres_virtual;
    int framebuffer_height = fb_info.yres_virtual;
    int framebuffer_depth  = fb_info.bits_per_pixel;
    printf( "xres_virtual : %d\n", fb_info.xres_virtual);
    printf( "yres_virtual : %d\n", fb_info.yres_virtual);
    printf( "bits_per_pixel : %d\n", fb_info.bits_per_pixel);

    int mode = -1;
    cout << "Input mode (1: detect all, 2: detect face, 3: training(offline), 4: training(online), 0: terminate)" << endl;
    cin >> mode;

    if (mode == 1){
        // pre-trained weight from haarcascade
        CascadeClassifier faceCascade, eyeCascade, smileCascade;
        faceCascade.load("./haarcascade_frontalface_alt2.xml");
        eyeCascade.load("./haarcascade_eye.xml");
        smileCascade.load("./haarcascade_smile.xml");
        
        // load face model (for testing)
        //Ptr<FaceRecognizer> model = EigenFaceRecognizer::create();
        //Ptr<FaceRecognizer> model = FisherFaceRecognizer::create();
        Ptr<FaceRecognizer> model = LBPHFaceRecognizer::create();
        model->read("./MyFaceLBPHModel.xml");

        namedWindow("detecting", WINDOW_AUTOSIZE);

        while(1){
            Mat frame;
            cap >> frame;

            // convert frame from RGB img to grayscale img and also do the resize
            Mat grayscale;
            cvtColor(frame, grayscale, COLOR_BGR2GRAY);
            resize(grayscale, grayscale, Size(grayscale.size().width / scale, grayscale.size().height / scale), INTER_AREA);
            equalizeHist(grayscale, grayscale);

            // detect faces in grayscale frame
            vector<Rect> faces;
            faceCascade.detectMultiScale(grayscale, faces, 1.1, minNeighbors, CASCADE_DO_ROUGH_SEARCH, faceSize);

            // for each detected faces, do something
            for(Rect area: faces){
                Mat face, face_resize;
                Point text_lb;

                // draw rectangle where face is
                if(area.height > 0 && area.width > 0){
                    Scalar drawColor = Scalar(255, 0, 0); // blue
                    int p1x = cvRound(area.x * scale);
                    int p1y = cvRound(area.y * scale);
                    int p2x = cvRound((area.x + area.width - 1) * scale);
                    int p2y = cvRound((area.y + area.height - 1) * scale);
                    rectangle(frame, Point(p1x, p1y), Point(p2x, p2y), drawColor);
                    face = grayscale(area);
                    text_lb = Point(p1x, p1y);
                }

                // detect eyes
                vector<Rect> eyes;
                eyeCascade.detectMultiScale(face, eyes);

                // show eyes
                for(Rect eye_area: eyes){
                    // draw rectangle where eye is
                    if(eye_area.height > 0 && eye_area.width > 0){
                        Scalar drawColor = Scalar(0, 255, 0); // green
                        int p1x = cvRound((area.x+eye_area.x) * scale);
                        int p1y = cvRound((area.y+eye_area.y) * scale);
                        int p2x = cvRound((area.x+eye_area.x + eye_area.width - 1) * scale);
                        int p2y = cvRound((area.y + eye_area.y + eye_area.height - 1) * scale);
                        rectangle(frame, Point(p1x, p1y), Point(p2x, p2y), drawColor);
                    }
                }

                // show smile
                vector<Rect> smiles;
                smileCascade.detectMultiScale(face, smiles);
                for(Rect smile_area: smiles){
                    // draw rectangle where eye is
                    if(smile_area.height > 0 && smile_area.width > 0){
                        Scalar drawColor = Scalar(0, 255, 255); // red
                        int p1x = cvRound((area.x+smile_area.x) * scale);
                        int p1y = cvRound((area.y+smile_area.y) * scale);
                        int p2x = cvRound((area.x+smile_area.x + smile_area.width - 1) * scale);
                        int p2y = cvRound((area.y + smile_area.y + smile_area.height - 1) * scale);
                        rectangle(frame, Point(p1x, p1y), Point(p2x, p2y), drawColor);
                    }
                }

                // do prediction
                int predict = 0;
                double confidence = 0.0;
                //cout << face.rows << " " << face.cols << endl;
                resize(face, face_resize, Size(img_width, img_height));
                if (!face_resize.empty()) model->predict(face_resize,predict,confidence);
                cout << predict << " " << confidence << endl;
                if(predict == 1 and confidence <= 80){
                    string name = "student1";
                    putText(frame, name, text_lb, FONT_HERSHEY_COMPLEX, 1, Scalar(0, 0, 255));
                }
                else if(predict == 2 and confidence <= 80){
                    string name = "student2";
                    putText(frame, name, text_lb, FONT_HERSHEY_COMPLEX, 1, Scalar(0, 0, 255));
                }
                else{
                    string name = "Unknown";
                    putText(frame, name, text_lb, FONT_HERSHEY_COMPLEX, 1, Scalar(0, 0, 255));
                }
                
            }

            // show the frame on screen
            imshow("detecting",frame);
            // show the frame for a period of time
            if(waitKey(10) >= 0){
                break;
            }                
        }
        destroyWindow("detecting");
    } else if (mode == 2) {
        // pre-trained weight from haarcascade
        CascadeClassifier faceCascade, eyeCascade, smileCascade;
        faceCascade.load("./haarcascade_frontalface_alt2.xml");
        eyeCascade.load("./haarcascade_eye.xml");
        smileCascade.load("./haarcascade_smile.xml");
        
        // load face model (for testing)
        //Ptr<FaceRecognizer> model = EigenFaceRecognizer::create();
        //Ptr<FaceRecognizer> model = FisherFaceRecognizer::create();
        Ptr<FaceRecognizer> model = LBPHFaceRecognizer::create();
        model->read("./MyFaceLBPHModel.xml");

        bool flag = false;
        clock_t start, end;
        double elapsed_time;
        start = clock();
        namedWindow("testing", WINDOW_AUTOSIZE);

        while(1){                
            Mat frame;
            cap >> frame;

            // convert frame from RGB img to grayscale img and also do the resize
            Mat grayscale;
            cvtColor(frame, grayscale, COLOR_BGR2GRAY);
            resize(grayscale, grayscale, Size(grayscale.size().width / scale, grayscale.size().height / scale), INTER_AREA);
            equalizeHist(grayscale, grayscale);

            // detect faces in grayscale frame
            vector<Rect> faces;
            faceCascade.detectMultiScale(grayscale, faces, 1.1, minNeighbors, CASCADE_DO_ROUGH_SEARCH, faceSize);

            // for each detected faces, do something
            for(Rect area: faces){
                Mat face, face_resize;
                Point text_lb;

                // draw rectangle where face is
                if(area.height > 0 && area.width > 0){
                    Scalar drawColor = Scalar(255, 0, 0); // blue
                    int p1x = cvRound(area.x * scale);
                    int p1y = cvRound(area.y * scale);
                    int p2x = cvRound((area.x + area.width - 1) * scale);
                    int p2y = cvRound((area.y + area.height - 1) * scale);
                    rectangle(frame, Point(p1x, p1y), Point(p2x, p2y), drawColor);
                    face = grayscale(area);
                    text_lb = Point(p1x, p1y);
                }

                // do prediction
                int predict = 0;
                double confidence = 0.0;
                //cout << face.rows << " " << face.cols << endl;
                resize(face, face_resize, Size(img_width, img_height));
                if (!face_resize.empty()) model->predict(face_resize,predict,confidence);
                cout << predict << " " << confidence << endl;
                if(predict == 1 && confidence <= 80){
                    end = clock();
                    elapsed_time = (double) (end-start);
                    string name = "student1(" + to_string(elapsed_time)+"ms)";
                    putText(frame, name, text_lb, FONT_HERSHEY_COMPLEX, 1, Scalar(0, 0, 255));
                    flag = true;
                }
                else if(predict == 2 && confidence <= 80){
                    end = clock();
                    elapsed_time = (double) (end-start);
                    string name = "student2(" + to_string(elapsed_time)+"ms)";
                    putText(frame, name, text_lb, FONT_HERSHEY_COMPLEX, 1, Scalar(0, 0, 255));
                    flag = true;
                }
            }
            // show the frame on screen
            imshow("testing",frame);
            waitKey(10);
            if(flag == true){
                break;
            }
        }
        destroyWindow("testing");
    } else if (mode == 3) {
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
        int test_count = 1;
        vector<Mat> testSamples;
        vector<int> testLabels;
        for(int i = 1;i <= test_count; i++){
            testSamples.push_back(images[images.size()-1]);
            testLabels.push_back(labels[labels.size()-1]);
            images.pop_back();
            labels.pop_back();
        }     
        
        // save face model (for training)
        Ptr<FaceRecognizer> model_Eigen = EigenFaceRecognizer::create();        
        Ptr<FaceRecognizer> model_Fisher = FisherFaceRecognizer::create();      
        Ptr<FaceRecognizer> model_LBPH = LBPHFaceRecognizer::create();

        model_Eigen->train(images, labels);
        //model_Eigen->update(images, labels);    
        model_Eigen->save("MyFaceEigenModel.xml");      
        model_Fisher->train(images, labels);
        //model_Fisher->update(images, labels);    
        model_Fisher->save("MyFaceFisherModel.xml");    
        model_LBPH->train(images, labels);
        //model_LBPH->update(images, labels);    
        model_LBPH->save("MyFaceLBPHModel.xml");   
      
        int predict_label;
        double confidence;
        int correct_Eigen = 0;
        int correct_Fisher = 0;
        int correct_LBPH  = 0;
        for(int i = 0;i < test_count; i++){
            model_Eigen->predict(testSamples[i],predict_label, confidence);
            if(predict_label == testLabels[i]){
                correct_Eigen++;
            }
            model_Fisher->predict(testSamples[i],predict_label, confidence);
            if(predict_label == testLabels[i]){
                correct_Fisher++;
            }
            model_LBPH->predict(testSamples[i],predict_label, confidence);
            if(predict_label == testLabels[i]){
                correct_LBPH++;
            }
        }
        cout << "Model Eigen accuracy: " << (double)correct_Eigen<< endl;
        cout << "Model Fisher accuracy: " << (double)correct_Fisher << endl;
        cout << "Model LBPH accuracy: " << (double)correct_LBPH << endl;
    } else if (mode == 4) {
        // pre-trained weight from haarcascade
        CascadeClassifier faceCascade, eyeCascade, smileCascade;
        faceCascade.load("./haarcascade_frontalface_alt2.xml");
        eyeCascade.load("./haarcascade_eye.xml");
        smileCascade.load("./haarcascade_smile.xml");

        // save face model (for training)
        //Ptr<FaceRecognizer> train_model = EigenFaceRecognizer::create();        
        //Ptr<FaceRecognizer> train_model = FisherFaceRecognizer::create();      
        Ptr<FaceRecognizer> train_model = LBPHFaceRecognizer::create();
        
        int training_frame, studentID;
        double precision;

        vector<Mat> images;
        vector<int> labels;

        cout << "Training two persons : " << endl;
        int persons = 2;
        while (persons > 0) {

            cout << "Enter studentID of training person: ";
            cin >> studentID;

            cout << "Enter number of training frame : ";
            cin >> training_frame;

            namedWindow("training", WINDOW_AUTOSIZE);

            Mat image_frame;
            for (int i = 0; i < training_frame; ++i) {
                Mat frame;
                cap >> frame;                

                // convert frame from RGB img to grayscale img and also do the resize
                Mat grayscale;
                cvtColor(frame, grayscale, COLOR_BGR2GRAY);
                resize(grayscale, grayscale, Size(grayscale.size().width / scale, grayscale.size().height / scale), INTER_AREA);
                equalizeHist(grayscale, grayscale);

                // detect faces in grayscale frame
                vector<Rect> faces;
                faceCascade.detectMultiScale(grayscale, faces, 1.1, minNeighbors, CASCADE_DO_ROUGH_SEARCH, faceSize);
                
                if (faces.size()) {
                    image_frame = frame(faces[i]);
                    // detect faces
                    for(Rect area: faces){
                        Mat face;
                        Point text_lb;

                        // draw rectangle where face is
                        if(area.height > 0 && area.width > 0){
                            Scalar drawColor = Scalar(255, 0, 0); // blue
                            int p1x = cvRound(area.x * scale);
                            int p1y = cvRound(area.y * scale);
                            int p2x = cvRound((area.x + area.width - 1) * scale);
                            int p2y = cvRound((area.y + area.height - 1) * scale);
                            rectangle(frame, Point(p1x, p1y), Point(p2x, p2y), drawColor);
                            face = grayscale(area);
                            text_lb = Point(p1x, p1y);
                        }
                    }

                    // training data
                    images.emplace_back(image_frame);
                    labels.emplace_back(studentID);
                    putText(frame, "training", Point(faces[0].x, faces[0].y), FONT_HERSHEY_SIMPLEX, 2, Scalar(255, 255, 0), 2);

                }               

                imshow("training", frame);
                // show the frame for a period of time
                if(waitKey(10) >= 0){
                    break;
                }   
            }          

            //train_model->train(images, labels);
            train_model->update(images, labels);            
            train_model->save("MyFaceLBPHModel.xml");
            train_model->predict(image_frame, studentID, precision);
            persons--;
        }
        destroyWindow("training");

        Ptr<FaceRecognizer> inference_model = LBPHFaceRecognizer::create();
        inference_model->read("./MyFaceLBPHModel.xml");

        namedWindow("testing", WINDOW_AUTOSIZE);

        while(1){                
            Mat frame;
            cap >> frame;

            // convert frame from RGB img to grayscale img and also do the resize
            Mat grayscale;
            cvtColor(frame, grayscale, COLOR_BGR2GRAY);
            resize(grayscale, grayscale, Size(grayscale.size().width / scale, grayscale.size().height / scale), INTER_AREA);
            equalizeHist(grayscale, grayscale);

            // detect faces in grayscale frame
            vector<Rect> faces;
            faceCascade.detectMultiScale(grayscale, faces, 1.1, minNeighbors, CASCADE_DO_ROUGH_SEARCH, faceSize);

            // for each detected faces, do something
            for(Rect area: faces){
                Mat face, face_resize;
                Point text_lb;

                // draw rectangle where face is
                if(area.height > 0 && area.width > 0){
                    Scalar drawColor = Scalar(255, 0, 0); // blue
                    int p1x = cvRound(area.x * scale);
                    int p1y = cvRound(area.y * scale);
                    int p2x = cvRound((area.x + area.width - 1) * scale);
                    int p2y = cvRound((area.y + area.height - 1) * scale);
                    rectangle(frame, Point(p1x, p1y), Point(p2x, p2y), drawColor);
                    face = grayscale(area);
                    text_lb = Point(p1x, p1y);
                }

                // do prediction
                int predict = 0;
                double confidence = 0.0;
                //cout << face.rows << " " << face.cols << endl;
                resize(face, face_resize, Size(img_width, img_height));
                if (!face_resize.empty()) inference_model->predict(face_resize,predict,confidence);
                cout << predict << " " << confidence << endl;
                if(predict == 1 and confidence <= 80){
                    string name = "student1";
                    putText(frame, name, text_lb, FONT_HERSHEY_COMPLEX, 1, Scalar(0, 0, 255));
                }
                else if(predict == 2 and confidence <= 80){
                    string name = "student2";
                    putText(frame, name, text_lb, FONT_HERSHEY_COMPLEX, 1, Scalar(0, 0, 255));
                }
                else{
                    string name = "Unknown";
                    putText(frame, name, text_lb, FONT_HERSHEY_COMPLEX, 1, Scalar(0, 0, 255));
                }
            }
            // show the frame on screen
            imshow("testing",frame);
            if(waitKey(10) >= 0){
                break;
            }   
        }
        destroyWindow("testing");
    } else {
        cout << "Terminate the execution ......" << endl;
    }
    return 0;
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