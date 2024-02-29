#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <fstream> // for std::ofstream
#include <string>
#include <ctime>
// this is C :/
#include <unistd.h>
#include <stdint.h> // for uint32_t
#include <sys/ioctl.h> // for ioctl
#include <linux/fb.h> // for fb_
#include <fcntl.h> // for O_RDWR
using namespace cv;
using namespace std;
using namespace face;

struct framebuffer_info { 
    uint32_t bits_per_pixel; uint32_t xres_virtual; 
};
struct framebuffer_info get_framebuffer_info(const char* framebuffer_device_path) {
    struct framebuffer_info info;
    struct fb_var_screeninfo screen_info;
    int fd = -1;
    fd = open(framebuffer_device_path, O_RDWR);
    if (fd >= 0) {
        if (!ioctl(fd, FBIOGET_VSCREENINFO, &screen_info)) {
            info.xres_virtual = screen_info.xres_virtual;
            info.bits_per_pixel = screen_info.bits_per_pixel;
        }
    }
    return info;
};
int main()
{
    // constant
    const double scale = 3.0;
    const int img_width = 138;
    const int img_height = 168;

    // pre-trained haarcascade
    CascadeClassifier faceCascade, eyeCascade, smileCascade;
    faceCascade.load("./haarcascade_frontalface_alt2.xml");
    eyeCascade.load("./haarcascade_eye.xml");
    smileCascade.load("./haarcascade_smile.xml");


    // face model
    //Ptr<FaceRecognizer> model = EigenFaceRecognizer::create();
    //Ptr<FaceRecognizer> model = FisherFaceRecognizer::create();
    Ptr<FaceRecognizer> model = LBPHFaceRecognizer::create();
    model->read("./MyFaceLBPHModel.xml");

    // video capture settings
    VideoCapture cap(2);
    //cap.set(CV_CAP_PROP_FRAME_WIDTH,frame_width);
    //cap.set(CV_CAP_PROP_FRAME_HEIGHT,frame_height);
    if(!cap.isOpened())
        return -1;


    while(1){
        int mode = -1;
        cout << "Input mode(1: detect, 2: perf. test, 3: end demo)" << endl;
        cin >> mode;
        if(mode == 1){
            // while-loop, keep reading & showing frames from web camera
            while(1){
                // get frame  from web camera
                Mat frame;
                cap >> frame;

                // convert frame from RGB img to grayscale img and also do the resize
                Mat grayscale;
                cvtColor(frame, grayscale, COLOR_BGR2GRAY);
                resize(grayscale, grayscale, Size(grayscale.size().width / scale, grayscale.size().height / scale),INTER_AREA);
                equalizeHist(grayscale, grayscale);

                // detect faces in grayscale frame
                vector<Rect> faces;
                faceCascade.detectMultiScale(grayscale, faces, 1.1, 3, CASCADE_DO_ROUGH_SEARCH, Size(50,50));


                // for each detected faces, do something
                for(Rect area: faces){
                    Mat face, face_resize;
                    Point text_lb;

                    // draw rectangle where face is
                    if(area.height > 0 && area.width > 0){
                        Scalar drawColor = Scalar(255, 0, 0); // blue
                        rectangle(frame, Point(cvRound(area.x * scale), cvRound(area.y * scale)),
                            Point(cvRound((area.x + area.width - 1) * scale), cvRound((area.y + area.height - 1) * scale)), drawColor);
                        face = grayscale(area);
                        text_lb = Point(cvRound(area.x * scale), cvRound(area.y * scale));
                    }
                    // detect eyes
                    vector<Rect> eyes;
                    eyeCascade.detectMultiScale(face, eyes);

                    // show eyes
                    for(Rect eye_area: eyes){
                        // draw rectangle where eye is
                        if(eye_area.height > 0 && eye_area.width > 0){
                            Scalar drawColor = Scalar(0, 255, 0); // green
                            rectangle(frame, Point(cvRound((area.x+eye_area.x) * scale), cvRound((area.y+eye_area.y) * scale)),
                                Point(cvRound((area.x+eye_area.x + eye_area.width - 1) * scale), cvRound((area.y + eye_area.y + eye_area.height - 1) * scale)), drawColor);
                        }
                    }
                    // show smile
                    vector<Rect> smiles;
                    smileCascade.detectMultiScale(face, smiles);
                    for(Rect smile_area: smiles){
                        // draw rectangle where eye is
                        if(smile_area.height > 0 && smile_area.width > 0){
                            Scalar drawColor = Scalar(0, 255, 255); // green
                            rectangle(frame, Point(cvRound((area.x+smile_area.x) * scale), cvRound((area.y+smile_area.y) * scale)),
                                Point(cvRound((area.x+smile_area.x + smile_area.width - 1) * scale), cvRound((area.y + smile_area.y + smile_area.height - 1) * scale)), drawColor);
                        }
                    }

                    // do prediction
                    int predict = 0;
                    double confidence = 0.0;
                    //cout << face.rows << " " << face.cols << endl;
                    resize(face, face_resize, Size(img_width, img_height));
                    if (!face_resize.empty()){
                        model->predict(face_resize,predict,confidence);
                    }
                    cout << predict << " " << confidence << endl;
                    if(predict == 1 and confidence <= 80){
                        string name = "Wensheng";
                        putText(frame, name, text_lb, FONT_HERSHEY_COMPLEX, 1, Scalar(0, 0, 255));
                    }
                    else if(predict == 2 and confidence <= 80){
                        string name = "SK";
                        putText(frame, name, text_lb, FONT_HERSHEY_COMPLEX, 1, Scalar(0, 0, 255));
                    }
                    else{
                        string name = "Unknown";
                        putText(frame, name, text_lb, FONT_HERSHEY_COMPLEX, 1, Scalar(0, 0, 255));
                    }
                    
                }

                // show the frame on screen
                imshow("Webcam Frame",frame);


                // wait a little time for an input ()
                if(waitKey(10) >= 0){
                    break;
                }
            }
        } // end if

        // performance test
        else if(mode == 2){
            bool end_flg = false;
            clock_t start, end; 
            start = clock();
            while(1){
                // get frame from web camera
                Mat frame;
                cap >> frame;
                // convert frame from RGB img to grayscale img and also do the resize
                Mat grayscale;
                cvtColor(frame, grayscale, COLOR_BGR2GRAY);
                resize(grayscale, grayscale, Size(grayscale.size().width / scale, grayscale.size().height / scale),INTER_AREA);
                equalizeHist(grayscale, grayscale);

                // detect faces in grayscale frame
                vector<Rect> faces;
                faceCascade.detectMultiScale(grayscale, faces, 1.1, 3, CASCADE_DO_ROUGH_SEARCH, Size(50,50));


                // for each detected faces, do something
                for(Rect area: faces){
                    Mat face, face_resize;
                    Point text_lb;

                    // draw rectangle where face is
                    if(area.height > 0 && area.width > 0){
                        Scalar drawColor = Scalar(255, 0, 0); // blue
                        rectangle(frame, Point(cvRound(area.x * scale), cvRound(area.y * scale)),
                            Point(cvRound((area.x + area.width - 1) * scale), cvRound((area.y + area.height - 1) * scale)), drawColor);
                        face = grayscale(area);
                        text_lb = Point(cvRound(area.x * scale), cvRound(area.y * scale));
                    }

                    // do prediction
                    int predict = 0;
                    double confidence = 0.0;
                    //cout << face.rows << " " << face.cols << endl;
                    resize(face, face_resize, Size(img_width, img_height));
                    if (!face_resize.empty()){
                        model->predict(face_resize,predict,confidence);
                    }
                    cout << predict << " " << confidence << endl;
                    double elapsed_time;
                    if(predict == 1 && confidence <= 80){
                        end = clock();
                        elapsed_time = (double)(end-start) / CLOCKS_PER_SEC * 1000.0;
                        string name = "Wensheng(" + to_string(elapsed_time)+")";
                        putText(frame, name, text_lb, FONT_HERSHEY_COMPLEX, 1, Scalar(0, 0, 255));
                        end_flg = true;
                    }
                    else if(predict == 2 && confidence <= 80){
                        end = clock();
                        elapsed_time = (double)(end-start) / CLOCKS_PER_SEC * 1000.0;
                        string name = "SK(" + to_string(elapsed_time)+")";
                        putText(frame, name, text_lb, FONT_HERSHEY_COMPLEX, 1, Scalar(0, 0, 255));
                        end_flg = true;
                    }
                }
                // show the frame on screen
                imshow("Webcam Frame",frame);
                waitKey(10);
                if(end_flg == true){
                    break;
                }
            }
        }

        else{
            cout << "End demo" << endl;
            break;
        }
    }// end while


    return 0;
}