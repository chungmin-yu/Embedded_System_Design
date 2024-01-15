#include <fcntl.h> 
#include <fstream>
#include <linux/fb.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <sys/ioctl.h>
#include <string> // for Advanced
#include <thread> // for Advanced
#include <mutex>  // for Advanced
#include <termios.h>
#include <unistd.h>

struct framebuffer_info
{
    uint32_t bits_per_pixel;    // framebuffer depth
    uint32_t xres_virtual;      // how many pixel in a row in virtual screen
    uint32_t yres_virtual;      // how many pixel in a column in virtual screen
};

struct framebuffer_info get_framebuffer_info(const char *framebuffer_device_path);

// LAB3 Advanced 1
// -------------------------------------------------------------------------------------------------------------------
bool left_flag = true;
bool right_flag = false;
bool end_flag = false;
std::mutex mutex_key, mutex_end;

void wait_key_input() {
    printf("Waiting key input ......\n");

    struct termios oldTIO, newTIO;
    tcgetattr(STDIN_FILENO, &oldTIO);
    newTIO = oldTIO;
    // disable canonical
    newTIO.c_lflag &= (~ICANON);
    tcflush(STDIN_FILENO, TCIFLUSH);
    tcsetattr(STDIN_FILENO, TCSANOW, &newTIO);

    while (true) {
        int input = ' ';
        read(STDIN_FILENO, &input, 1);
        //printf("Key input = %d\n", input);        
        if (input == 'j') {
            mutex_key.lock();
            left_flag = 1;
            right_flag = 0;
            mutex_key.unlock();
        }
        else if (input == 'l') {
            mutex_key.lock();
            right_flag = 1;
            left_flag = 0;
            mutex_key.unlock();
        }
        else if (input == 'x') {
            mutex_end.lock();
            end_flag = 1;
            mutex_end.unlock();
            break;
        }
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldTIO);
}
// -------------------------------------------------------------------------------------------------------------------


int main(int argc, const char *argv[])
{
    if (strcmp(argv[1], "basic") == 0) {
        // LAB3 Advanced 1
        // ---------------------------------------------------------------------------------------------------------------
        // create thread for waiting key input
        // std::thread t1(wait_key_input);
        // int screenshot_count = 0;
        // ---------------------------------------------------------------------------------------------------------------

        // setting the parameters of the video stream
        const int frame_width = 1280; 
        const int frame_height = 720;
        const int frame_rate = 30;

        cv::Mat frame;
        cv::Mat tmp_frame;
        cv::Size2f frame_size;

        // transfer color space from BGR to BGR565 (16-bit image) to fit the requirement of the LCD
        // https://docs.opencv.org/3.4.7/d8/d01/group__imgproc__color__conversions.html#ga397ae87e1288a81d2363b61574eb8cab
        // https://docs.opencv.org/3.4.7/d8/d01/group__imgproc__color__conversions.html#ga4e0972be5de079fed4e3a10e24ef5ef0
        cv::Mat framebuffer_BGR565;
        cv::Mat tmp_image_BGR565;


        // open video stream device
        // https://docs.opencv.org/3.4.7/d8/dfe/classcv_1_1VideoCapture.html#a5d5f5dacb77bbebdcbfb341e3d4355c1
        cv::VideoCapture cap (2);

        framebuffer_info fb_info = get_framebuffer_info("/dev/fb0");
        std::ofstream ofs("/dev/fb0");

        // check if video stream device is opened success or not
        // https://docs.opencv.org/3.4.7/d8/dfe/classcv_1_1VideoCapture.html#a9d2ca36789e7fcfe7a7be3b328038585
        if(!cap.isOpened()) {
            printf("Could not open video device.n");
            return 1;
        }

        cap.set(CV_CAP_PROP_FRAME_WIDTH, frame_width); // fb_info.xres_virtual
        cap.set(CV_CAP_PROP_FRAME_HEIGHT, frame_height); // fb_info.yres_virtual
        cap.set(CV_CAP_PROP_FPS, frame_rate); 

        // LAB3 Advanced 2
        // ---------------------------------------------------------------------------------------------------------------
        // cv::Size video_size = cv::Size((int)cap.get(CV_CAP_PROP_FRAME_WIDTH), (int)cap.get(CV_CAP_PROP_FRAME_HEIGHT));
        // std::string PATH = "/run/media/mmcblk1p1/";
        // std::string video_path = PATH + "screenshot/advanced.avi";
        // cv::VideoWriter writer(video_path, CV_FOURCC('M','J','P','G'), frame_rate, video_size, true);
        // ---------------------------------------------------------------------------------------------------------------

        int framebuffer_width  = fb_info.xres_virtual;
        int framebuffer_height = fb_info.yres_virtual;
        int framebuffer_depth  = fb_info.bits_per_pixel;
        printf( "xres_virtual : %d\n", fb_info.xres_virtual);       // 1920
        printf( "yres_virtual : %d\n", fb_info.yres_virtual);       // 1080
        printf( "bits_per_pixel : %d\n", fb_info.bits_per_pixel);

        int test_count = 0;
        while (true) {
            //cap >> tmp_frame;
            cap >> frame;
            //cv::resize(tmp_frame, frame, cv::Size(frame_width, frame_height), cv::INTER_LINEAR);

            // LAB3 Advanced 1
            // -----------------------------------------------------------------------------------------------------------
            // writer.write(frame);
            // if(key_flag == true) {
            //     std::string screenshot_path = PATH + "screenshot/screenshot" + std::to_string(screenshot_count) + ".png";
            //     cv::imwrite(screenshot_path, frame);
            //     screenshot_count++;
            //     //printf("Screenshot successfully.\n");
            //     mutex_key.lock();
            //     key_flag = false;
            //     mutex_key.unlock();
            // }
            // if(end_flag == true) {
            //     printf("Recording ends.\n");
            //     cap.release();
            //     writer.release();
            //     t1.join();
            //     return 0;
            // }
            // -----------------------------------------------------------------------------------------------------------

            // get image size of the image.
            // https://docs.opencv.org/3.4.7/d3/d63/classcv_1_1Mat.html#a146f8e8dda07d1365a575ab83d9828d1
            frame_size = frame.size();

            cv::cvtColor(frame, framebuffer_BGR565, cv::COLOR_BGR2BGR565);
            for (int y = 0; y < frame_size.height; y++) {
                // 2 is because one pixel has two byte (fb_info.bits_per_pixel/8)

                // move to the next written position of output device framebuffer by "std::ostream::seekp()".
                // posisiotn can be calcluated by "y", "fb_info.xres_virtual", and "fb_info.bits_per_pixel".
                // http://www.cplusplus.com/reference/ostream/ostream/seekp/
                ofs.seekp(180*framebuffer_width*2 + y*framebuffer_width*2 + 320*2);

                // write to the framebuffer by "std::ostream::write()".
                // you could use "cv::Mat::ptr()" to get the pointer of the corresponding row.
                // you also have to count how many bytes to write to the buffer
                // http://www.cplusplus.com/reference/ostream/ostream/write/
                // https://docs.opencv.org/3.4.7/d3/d63/classcv_1_1Mat.html#a13acd320291229615ef15f96ff1ff738
                ofs.write(reinterpret_cast<char*>(framebuffer_BGR565.ptr(y)), frame_size.width*2);
            }
            test_count++;
        }
    } else {
        // LAB4 advanced
        cv::Mat image;
        cv::Size2f image_size;

        // create thread for waiting key input
        std::thread t1(wait_key_input);
        
        framebuffer_info fb_info = get_framebuffer_info("/dev/fb0");
        std::ofstream ofs("/dev/fb0");

        // read image file (sample.bmp) from opencv libs.
        image= cv::imread("picture.png");     // 3840*1080

        // get image size of the image.
        image_size = image.size();

        // transfer color space from BGR to BGR565 (16-bit image) to fit the requirement of the LCD
        cv::Mat image_BGR565;
        cv::cvtColor(image, image_BGR565, cv::COLOR_BGR2BGR565);

        // output to framebufer row by row
        int framebuffer_width = fb_info.xres_virtual;
        int framebuffer_height = fb_info.yres_virtual;
        int framebuffer_depth = fb_info.bits_per_pixel;
        printf( "xres_virtual : %d\n", fb_info.xres_virtual);       // 1920
        printf( "yres_virtual : %d\n", fb_info.yres_virtual);       // 1080
        printf( "bits_per_pixel : %d\n", fb_info.bits_per_pixel);

        // electronic scroll board
        int x = 0;
        int write_width;
        int write_width_patch;
        while(true) {
            if(end_flag == true) {
                t1.join();
                return 0;
            }

            // print electronic scroll board
            // move left --> right patch
            write_width = framebuffer_width;
            for (int y = 0; y < image_size.height ; y++) {
                // 2 is because one pixel has two byte (fb_info.bits_per_pixel/8)
                ofs.seekp(y*framebuffer_width*2);
                ofs.write(reinterpret_cast<char*>(image_BGR565.ptr(y, x)), write_width*2);
            }
            if (left_flag == true) {
                x+=40;
            }
            else if (right_flag == true) {
                if (x==0) x=3840;
                x-=40;                
            }

        }

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