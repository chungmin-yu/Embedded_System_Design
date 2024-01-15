#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char **argv) {

    char c;
    FILE *fp;
    fp = fopen(argv[1], "wb");
    printf("Please enter transferred file size: ");
    
    int size = 0;
    scanf("%d", &size);
    
    printf("File size : %d\n", size);
    getchar();

    int time1 = (int)time(NULL);
    while (size--) {

        int c = getchar();
        fwrite(&c, sizeof(char), 1, fp);
    }
    int time2 = (int)time(NULL);



    fclose(fp);
    printf("Used time: %d s\n", time2-time1);
}