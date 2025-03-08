#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "concurrent_list.h"

struct node
{
    int value;
    node* next;
    pthread_mutex_t Lock;
};
struct list
{
    node* head;
    pthread_mutex_t Lock;
};
list* create_list()
{
    list* myList= ((list*)malloc(sizeof(list)));
    if(myList!=NULL)  /*check if malloc failed*/
    {
        myList->head=NULL;
        if((pthread_mutex_init(&myList->Lock,NULL))!= 0)
            exit(-1);
        return myList;
    }
    else
    {
        printf("malloc failed!\n");
        exit(-1);
    }

}
void print_node(node* node)
{
    /* DO NOT DELETE*/
    if(node)
    {
        printf("%d ", node->value);
    }
}
void delete_list(list* list)
{
    node* temp = NULL;
    node* current = NULL;

    if (!list) /*if there is no list*/
    {
        return;
    }

    if(list->head)
    {
        pthread_mutex_lock(&list->head->Lock);
        current = list->head;

        while(current->next != NULL)
        {
            pthread_mutex_lock(&current->next->Lock);
            temp = current;
            current = current->next;

            pthread_mutex_unlock(&temp->Lock);
            if((pthread_mutex_destroy(&temp->Lock))!= 0)
                exit(-1);
            free(temp);
        }

        pthread_mutex_unlock(&current->Lock);
        if((pthread_mutex_destroy(&current->Lock))!=0)
            exit(-1);
        free(current);
    }

    free(list);
}
node* create_node(int value)
{
    node* newNode= ((node*)malloc(sizeof(node)));
    if(newNode!=NULL)  /*check if malloc failed*/
    {
        newNode->value = value;
        newNode->next = NULL;
        if((pthread_mutex_init(&newNode->Lock, NULL))!= 0 )
            exit(-1);

        return newNode;
    }
    else
    {
        printf("malloc failed!\n");
        exit(-1);
    }
}
void insert_value(list* list, int value)
{
    node* curr;
    node* prev;

    if (!list)
        return;

    /*create new node*/
    node* new_node = create_node(value);
    pthread_mutex_lock(&new_node->Lock);

    /*if the linked list is empty or the value of the node to be inserted is smaller than the
    value of the head node, then insert the node at the start and make it head.*/
    if((!list->head) || (new_node->value <= list->head->value))
    {
        if(!list->head) /* if the list is empty */
        {
            pthread_mutex_lock(&list->Lock);
            list->head = new_node;
            pthread_mutex_unlock(&new_node->Lock);
            pthread_mutex_unlock(&list->Lock);
        }
        else
        {
            pthread_mutex_lock(&list->Lock);
            new_node->next = list->head;
            list->head = new_node;
            pthread_mutex_unlock(&new_node->Lock);
            pthread_mutex_unlock(&list->Lock);
        }
        return;
    }

    pthread_mutex_lock(&list->head->Lock);
    curr=list->head;

    /*find the appropriate node where the input will be inserted.*/
    while(new_node->value > curr->value && curr->next != NULL)
    {
        pthread_mutex_lock(&curr->next->Lock);
        prev=curr;
        curr=curr->next;
        if((new_node->value > curr->value) || (new_node->value >= curr->value && !curr->next))
            pthread_mutex_unlock(&prev->Lock);
    }

    /*insert the node at the end */
    if(!curr->next && new_node->value >= curr->value)
    {
        curr->next = new_node;
        pthread_mutex_unlock(&new_node->Lock);
        pthread_mutex_unlock(&curr->Lock);
        return;
    }

    /* insert the node in the middle */
    new_node->next=curr;
    prev->next=new_node;

    /*unlock mutex*/
    pthread_mutex_unlock(&new_node->Lock);
    pthread_mutex_unlock(&prev->Lock);
    pthread_mutex_unlock(&curr->Lock);
}

void remove_value(list* list, int value)
{
    if (!list || !list->head)
    {
        return;
    }

    node* temp = NULL;
    node* prev = NULL;

    pthread_mutex_lock(&list->head->Lock);
    temp=list->head;

    if(temp->value == value && temp != NULL) /*If the head node itself holds the key to be deleted*/
    {

        list->head = list->head->next; /*change head*/
        pthread_mutex_unlock(&temp->Lock);

        if((pthread_mutex_destroy(&temp->Lock))!= 0)
            exit(-1);
        free(temp);

        return;
    }

    /* Search for the key to be deleted*/
    while (temp->value != value)
    {
        if(temp->next != NULL)
        {
            pthread_mutex_lock(&temp->next->Lock);
            prev = temp;
            temp = temp->next;
            pthread_mutex_unlock(&prev->Lock);
        }

        /*if key was not present in linked list*/
        else
        {
            pthread_mutex_unlock(&temp->Lock);
            return;
        }
    }

    /* unlink the node from linked list*/
    pthread_mutex_lock(&prev->Lock);
    prev->next = temp->next;
    pthread_mutex_unlock(&prev->Lock);

    /*unlock mutex and free memory*/
    pthread_mutex_unlock(&temp->Lock);
    if((pthread_mutex_destroy(&temp->Lock))!= 0)
        exit(-1);
    free(temp);
}

void print_list(list* list)
{
    if(list!=NULL && list->head != NULL)
    {
        node* temp = NULL;
        node* printed_node = NULL;

        pthread_mutex_lock(&list->head->Lock);
        temp=list->head;

        while(temp)
        {
            print_node(temp);
            if(temp->next)
                pthread_mutex_lock(&temp->next->Lock);
            printed_node=temp;
            temp=temp->next;
            pthread_mutex_unlock(&printed_node->Lock);
        }
    }
    printf("\n"); // DO NOT DELETE
}

void count_list(list* list, int (*predicate)(int))
{
    int count = 0; /* DO NOT DELETE*/

    node* temp1 = NULL;
    node* temp2 = NULL;
    node* current = NULL;

    if(!list->head || !list) /*if the list is empty or there is no list*/
    {
        printf("%d items were counted\n", count);
        return;
    }

    pthread_mutex_lock(&list->head->Lock);
    current=list->head;

    while(current)
    {
        if (predicate(current->value))
            count++;
        temp1=current->next;

        if(temp1)
            pthread_mutex_lock(&temp1->Lock);
        temp2=current;
        current=temp1;
        pthread_mutex_unlock(&temp2->Lock);

    }

    printf("%d items were counted\n", count); /* DO NOT DELETE*/
}

