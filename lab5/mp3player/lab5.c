#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>

#define MAX_LENGTH 256

struct Node
{
	char* name;
	struct Node* next;
 	struct Node* prev;
};

struct Node* head = NULL;
struct Node* tail = NULL;

// insert a newNode at the end of the list
void pushback(char* data) {

	struct Node* new_node = (struct Node*)malloc(sizeof(struct Node));
	new_node->name = malloc(sizeof(char) * MAX_LENGTH);
  strcpy(new_node->name, data);
  new_node->next = NULL;
  new_node->prev = NULL;

  if (tail == NULL) {
      head = new_node;
      tail = new_node;
  } else {
      new_node->prev = tail;
      tail->next = new_node;
      tail = new_node;
  }
}

// delete a node from the doubly linked list
void removeback() {
 if (head == NULL) {
        return;
    }
    struct Node* temp = head;
    if (head == tail) {
        head = NULL;
        tail = NULL;
    } else {
        head = head->next;
        head->prev = NULL;
    }
    free(temp);
}

// print the doubly linked list
void displayList() {
  struct Node* current = head;
  while (current != NULL) {
        printf("%s ", current->name);
        current = current->next;
    }
    printf("\n");
}

int main(int argc, char *argv[])
{
	//default setting
	const char songpath[] = "songlib";
	
	pushback("song1");
	struct Node* current_song = head;
	char *current_songName = current_song->name;

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
				pushback(songName);              	
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
				displayList();
				break;
			case 'b':
				printf("back\n");
				if (current_song->next != NULL) {
					current_song = current_song->next;
				}				
				current_songName = current_song->name;
				//system("killall -9 madplay");
				printf("killall -9 madplay\n");
				sprintf(command, "madplay %s/%s &", songpath, current_songName);
				printf("%s\n", command);
				//system(command);
				break;
			case 'f':
				printf("forward\n");
				if (current_song->prev != NULL) {
					current_song = current_song->prev;
				}				
				current_songName = current_song->name;
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