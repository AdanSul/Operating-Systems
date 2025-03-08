#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE 100

typedef struct node
{
    char content[BUFFER_SIZE];
    int key;
    struct node* prev;
} command_node;



int main(void)
{
    close(2);
    dup(1);

    char command[BUFFER_SIZE];
    char* command_arr[BUFFER_SIZE+1];
    command_node* history = NULL;
    command_node* temp = NULL;
    command_node* tmp = NULL;
    int counter1=1,counter2;
    int status,num;
    int background = 0;
    int flag=0;

    while (1)
    {
        background=0;
        counter2=0;
        flag=0;
        fprintf(stdout, "my-shell> ");
        memset(command, 0, BUFFER_SIZE);
        fgets(command, BUFFER_SIZE, stdin);
        if(strncmp(command, "exit", 4) == 0)
        {
            break;
        }

        if(strncmp(command, "history", 7) != 0)
        {
            command_node* new_node = malloc(sizeof (command_node));  /* create new node*/
            strcpy(new_node->content, command);
            new_node->key = counter1;
            new_node->prev = history;
            counter1++;
            history = new_node;

            char deli[] = " \n\t\r";   /* the delimiters are space, \r, tab and new line*/
            char* token = strtok(command,deli);

            if((token != NULL) && (history!=NULL) && (strncmp(token, "!", 1) == 0)) /* check if the first char is !*/
            {
                flag=1;
                tmp = history;
                if (strlen(token) == 2 && strncmp(token + 1, "!", 1) == 0) /* checking for !! */
                {
                    if (!tmp)
                    {
                        perror("error");
                        exit(EXIT_FAILURE);
                    }
                    flag=1;
                    strcpy(command, tmp->prev->content);
                    strcpy(history->content, tmp->prev->content);
                    token = strtok(command, deli);
                }
                else
                {
                    int num = atoi(token + 1); /* convert the string to an integer */
                    if (num > counter1-2 )     /* !x while x grater than the commands in history */
                    {
                        tmp=history->prev;
                        history->prev=NULL;
                        history=tmp;
                        counter1--;
                        printf("No History\n");
                        continue;

                    }

                    while(tmp != NULL)
                    {
                        if(tmp->key == num)  /* if the number is found, copy its content to the to the node and to the current command */
                        {
                            strcpy(command, tmp->content);
                            strcpy(history->content, tmp->content);
                            token = strtok(command, deli);
                            break;
                        }
                        tmp = tmp->prev;
                    }
                }
            }

            while(token!=NULL && (counter2 < BUFFER_SIZE -1))
            {
                command_arr[counter2]=token;
                counter2++;
                token=strtok(NULL,deli);
            }

            if (counter2 == (BUFFER_SIZE - 1) && token != NULL)
            {
                fprintf(stdout, "error: command too long\n");
                continue;
            }

            command_arr[counter2] = NULL;

            if(counter2-1!=0)
            {
                if(strncmp(command_arr[counter2-1],"&",1)==0) /*check if back/fore ground */
                {
                    background=1;
                    counter2--;
                    command_arr[counter2] = NULL;
                }
            }


            pid_t pid = fork();

            if (pid < 0)  /*if the fork failed*/
            {
                perror("error");
                exit(EXIT_FAILURE);
            }

            if (pid == 0)  /*son process*/
            {
                if(strncmp(command, "history", 7) != 0)
                {
                    if (execvp(command_arr[0], command_arr) == -1)
                    {
                        perror("error");
                        exit(EXIT_FAILURE);
                    }
                }
                exit(0);

            }

            if((pid>0) && (background==0))    /*if we are in the father process and there are foreground running*/
            {
                if (waitpid(pid, &status, 0) == -1)
                {
                    perror("error");
                    exit(EXIT_FAILURE);
                }
            }
        }

        if(strncmp(command, "history", 7) == 0)  /*check if the command is history*/
        {
            if(!flag)
            {
                command_node* new_node = malloc(sizeof (command_node));  /*create a new node*/
                strcpy(new_node->content, command);
                new_node->key = counter1;
                new_node->prev = history;
                counter1++;
                history = new_node;
            }

            temp = history;     /*history points to the last node in the list*/

            while (temp!= NULL)  /*print history*/
            {
                printf("%d \t %s", temp->key,temp->content);
                temp = temp->prev;
            }
        }
    }
    return 0;
}
