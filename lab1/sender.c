#include <stdio.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdbool.h>
#include <string.h>
#define buffer_size 200

struct termios_options {
    int serial_fd;
    struct termios options;
};

int main(void) {
    FILE *fptr;
    char buffer[buffer_size];

    fptr = fopen("./base64_encoded_file.txt","rb");
    if (fptr == NULL) {
        perror("Can't find transferred file!\n");
        return -1;
    }

    // connect device using termios
    struct termios_options opt;
    opt.serial_fd = -1;
    opt.serial_fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY );

    if(opt.serial_fd == -1) {
        perror("Fail to open /dev/ttyUSB0 device!\n");
        return -1; 
    }

    // get the parameters about the terminal
    tcgetattr(opt.serial_fd, &opt.options);

    // setting the flags
    opt.options.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
    opt.options.c_iflag = IGNPAR;
    opt.options.c_oflag = 0;
    opt.options.c_lflag = 0;
    
    // clean the current setting
    tcflush(opt.serial_fd, TCIFLUSH);
    // Enable the new setting right now
    tcsetattr(opt.serial_fd, TCSANOW, &opt.options);

    // transfer file
    while (feof(fptr) == 0) {
        fscanf(fptr, "%s", buffer); 
        //printf("%s\n",buffer);
        write(opt.serial_fd, buffer, strlen(buffer));
        write(opt.serial_fd, "\n", 1);
    }

    return 0;
}