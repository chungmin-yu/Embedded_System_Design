#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <dirent.h> 
#include <list>

using namespace std;
#define MAX_LENGTH 256

struct song
{
	char name[MAX_LENGTH];
};

int main(int argc, char *argv[])
{
	//default setting
	const char songpath[] = "songlib";
	std::list<song> songList;
	struct song default_song;
	strcpy(default_song.name, "song1");
	songList.push_back(default_song);
	std::list<song>::iterator current_songIter = songList.begin();
	char *current_songName = current_songIter->name;

	while (true) {
		bool exit_flag = false;
		char songName[MAX_LENGTH] = {0};
		char command[MAX_LENGTH] = {0};
		char c = getchar();
		switch (c)
		{
			case 'a':
				printf("Please enter the song name: ");
				scanf("%s", songName);
				printf("add %s\n", songName);
				struct song add_song;
				strcpy(add_song.name, songName);
				songList.push_back(add_song);               	
				break;
			case 'p':
				printf("play %s\n", current_songName);
				sprintf(command, "madplay %s/%s &", songpath, current_songName);
				printf("%s\n", command);
				//system(command);
				break;
			case 's':
				printf("stop\n");
				//system("killall -9 madplay");
				printf("killall -9 madplay\n");
				break;
			case 'l':
				printf("Playlist: ");
				for (std::list<song>::iterator it = songList.begin(); it != songList.end(); it++) {
					printf("%s, ", it->name);
				}
				printf("\n");
				break;
			case 'b':
				printf("back\n");
				current_songIter++;
				if (current_songIter == songList.end()) {
					current_songIter--;
				}
				current_songName = current_songIter->name;
				//system("killall -9 madplay");
				printf("killall -9 madplay\n");
				sprintf(command, "madplay %s/%s &", songpath, current_songName);
				printf("%s\n", command);
				//system(command);
				break;
			case 'f':
				printf("forward\n");
				if (current_songIter != songList.begin()) {
					current_songIter--;
				} else {
					current_songIter = songList.begin();
				}
				current_songName = current_songIter->name;
				//system("killall -9 madplay");
				printf("killall -9 madplay\n");
				sprintf(command, "madplay %s/%s &", songpath, current_songName);
				printf("%s\n", command);
				//system(command);
				break;
			case 'e':
				printf("exit\n");
				exit_flag = true;
				break;
		}
		
		if (exit_flag) {
			break;
		}
	}
}