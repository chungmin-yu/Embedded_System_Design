#include <stdio.h>
#include <stdlib.h>

struct Node {
    int data;
    struct Node* next;
    struct Node* prev;
};

struct Node* head = NULL;
struct Node* tail = NULL;

// create a new node with the given data and return a pointer to it

struct Node* create_node(int data) {
    struct Node* new_node = (struct Node*)malloc(sizeof(struct Node));
    new_node->data = data;
    new_node->next = NULL;
    new_node->prev = NULL;
    return new_node;
}

// insert a node at the beginning of the list

void insert_at_head(int data) {
    struct Node* new_node = create_node(data);
    if (head == NULL) {
        head = new_node;
        tail = new_node;
    } else {
        new_node->next = head;
        head->prev = new_node;
        head = new_node;
    }
}

// insert a node at the end of the list

void insert_at_tail(int data) {
    struct Node* new_node = create_node(data);
    if (tail == NULL) {
        head = new_node;
        tail = new_node;
    } else {
        new_node->prev = tail;
        tail->next = new_node;
        tail = new_node;
    }
}

// delete the node at the beginning of the list

void delete_at_head() {
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

// delete the node at the end of the list

void delete_at_tail() {
    if (tail == NULL) {
        return;
    }
    struct Node* temp = tail;
    if (head == tail) {
        head = NULL;
        tail = NULL;
    } else {
        tail = tail->prev;
        tail->next = NULL;
    }
    free(temp);
}

// display the list in forward direction

void display_forward() {
    struct Node* current = head;
    while (current != NULL) {
        printf("%d ", current->data);
        current = current->next;
    }
    printf("\n");
}

// display the list in backward direction

void display_backward() {
    struct Node* current = tail;
    while (current != NULL) {
        printf("%d ", current->data);
        current = current->prev;
    }
    printf("\n");
}

// main function to test the doubly linked list

int main() {
    insert_at_head(1);
    insert_at_head(2);
    insert_at_tail(3);
    display_forward(); // expected output: 2 1 3
    display_backward(); // expected output: 3 1 2
    delete_at_head();
    delete_at_tail();
    display_forward(); // expected output: 1
    display_backward(); // expected output: 1
    return 0;
}


