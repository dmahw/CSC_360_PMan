/****************************
    David Mah
    CSC 360 - A02 - T03

    Assignment 1
****************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

unsigned int numOfBG = 0;   //Used to store number of background processes

/*********************
    Linked List implementation from
    https://www.tutorialspoint.com/data_structures_algorithms/linked_list_program_in_c.htm

    Parts of the linked list implmentation in the link above has been
    modified for this assignment, not all code listed in the link is included.
*********************/
struct node {
    int id;
    char *name;
    int state;
    struct node *next;
};

struct node *head = NULL;
struct node *current = NULL;

void printList() {
    struct node *ptr = head;
    while(ptr != NULL) {
        printf("%d: %s %d\n", ptr->id, ptr->name, ptr->state);
        ptr = ptr->next;
    }
}

void insertProcess(int id, char *name, int state) {
    struct node *link = (struct node*) malloc(sizeof(struct node));
    numOfBG++;
    link->id = id;
    link->name = malloc(sizeof(name));
    link->name = name;
    link->state = state;
    link->next = head;
    head = link;
}

struct node* deleteProcess(int id) {
    struct node* current = head;
    struct node* previous = NULL;  
    if(head == NULL) {
        return NULL;
    }
    while(current->id != id) {
        if(current->next == NULL) {
            return NULL;
        } else {
            previous = current;
            current = current->next;
        }
    }
    if(current == head) {
        --numOfBG;
        head = head->next;
    } else {
        --numOfBG;
        previous->next = current->next;
    }
    return current;
}

//Used to store whether the program is running or not
struct node* changeState(int id, int state) { 
    struct node* current = head;
    if(head == NULL) {
        return NULL;
    }
    while(current->id != id) {
        if(current->next == NULL) {
            return NULL;
        } else {
            current = current->next;
        }
    }
    current->state = state;
    return current;
}
/**************************************
    END OF LINKED LIST IMPLEMENTATION
**************************************/

int bg(char *input[]) {
    pid_t pid = fork();                                 //Fork
    unsigned int processID = pid;
    if (pid == 0) {                                     //Child Process
        setpgid(0, 0);                                  //Sets the program in a different process group
        execvp(input[1], &input[1]);                    //Executes given command with arguments
        printf("Error: Failed to start process.\n");    //Error message only prints if exec fails
        exit(1);                                        //Only exits the child process if exec fails.
    } else if (pid > 0) {                               //Parent Process
        insertProcess(processID, input[1], 1);          //Insert process into linked list
        sleep(1);
        return 0;                                       //Parent process returns;
    } else {                                            //Failure of fork
        printf("Error: Failed to fork process.\n");
        return 1;
    }
}

int bglist(){
    printList();                                        //Prints all current background process stored in a linked list
    printf("Total background jobs: %d\n", numOfBG);
    return 0;
}

int bgkill(int id){
    if(kill(id, SIGKILL) != 0) {
        printf("Error: Failed to kill process %d\n", id);   //Prints only if kill fails to kill specified process
    }
    sleep(1);
    deleteProcess(id);
    return 0;
}

int bgstop(int id){
    if(kill(id, SIGSTOP) == 0) {
        changeState(id, 0);                             //Changes state recorded in the linked list
    } else {
        printf("Error: Failed to stop process %d\n", id);   //If it us unable to stop the specified process
    }
    return 0;
}

int bgstart(int id){
    if(kill(id, SIGCONT) == 0) {    
        changeState(id, 1);                             //Changes state recorded in the linked list
    } else {
        printf("Error: Failed to start process %d\n", id);  //If it is unable to start the specified process
    }
    return 0;
}

int pstat(int id){
    char *path1 = malloc(32);                           //Path memory allocation for /proc/[pid]/stat
    char *path2 = malloc(32);                           //Path memory allocation for /proc/[pid]/stat
    char *line = malloc(512);                           //Used to store the line for each status and stat

    sprintf(path1, "/proc/%d/stat", id);                //Path concatenation with specified pid
    sprintf(path2, "/proc/%d/status", id);

    FILE* file1 = fopen(path1, "r");                    //Open files
    FILE* file2 = fopen(path2, "r");

    if (file1 && file2) {                               //Check opening of files, if NULL, unable to locate path
        fgets(line, 1024, file1);                       //Reads each line of stat, in this case, just 1 line
        int counter = 0;                                //Used to specify which data value to read
        char *token = strtok(line, " ");                //Each value is seperated by tokenizing the string with spaces
        while (token != NULL) {                         //For each token, if specified value, print the following values: comm, state, utime, stime, rss
            if (counter == 1) printf("comm:\t%s\n", token); 
            else if (counter == 2) printf("state:\t%s\n", token);    
            else if (counter == 13) printf("utime:\t%lu\n", strtol(token, NULL, 10)/sysconf(_SC_CLK_TCK));
            else if (counter == 14) printf("stime:\t%lu\n", strtol(token, NULL, 10)/sysconf(_SC_CLK_TCK));
            else if (counter == 23) {
                printf("rss:\t%s\n", token);
                break;
            }
            token = strtok(NULL, " ");
            ++counter;
        }

        while(fgets(line, 128, file2)) {                //For each line in status
            if(strncmp(line, "voluntary_ctxt_switches:", 24) == 0) {    //Checks each line until specified string is found
                printf("%s", line);
            } else if(strncmp(line, "nonvoluntary_ctxt_switches:", 27) == 0) {
                printf("%s", line);
            }
        }
        fclose(file1);                                  //Closes files
        fclose(file2);
    } else {
        printf("Error: Process %d does not exist\n", id);   //If unable to access file, prints the following error
    }
    return 0;
}

int checkForZombies() {                                 //Checks for any zombies processes
    int *status = 0;
    int pid = waitpid(-1, status, WNOHANG);             //Checks all pid for zombies
    if(pid > 0) {                                       //Continues to check until pid returned is less than or equal to 0
        deleteProcess(pid);                             //Delete process from linked list
        checkForZombies();                              //Continuously check
    }
    return 0;
}

int errCorrGenUsage() {                                 //Prints if the entered command does not match any supported commands
    printf("USAGE:\n\t[bg] [program] [args]\n\t[bglist]\n\t[bgkill] [id]\n\t[bgstop] [id]\n\t[bgstart] [id]\n\t[pstat] [id]\n");
    return 0;
}

int main(int argc, char* argv[]) {
    while (1) {
        printf("PMan: >");                              //PMan prompt

        char *userInput = malloc(512);                  //Memory allocation for user input
        char *input[1024];
        fgets(userInput, 4096, stdin);

        char *token = strtok(userInput, " \t\n\v\f\r"); //Reference: https://stackoverflow.com/questions/15472299/split-string-into-tokens-and-save-them-in-an-array
        int counter = 0;
        while(token != NULL) {                          //For each token, put it in an array
            input[counter++] = token;
            token = strtok(NULL, " \t\n\v\f\r");
        }

        if (strcmp(input[0], "bg") == 0) bg(input);     //Check which command specified, and go to the following switches
        else if (strcmp(input[0], "bglist") == 0) {
            checkForZombies();                          //Before printing the list, check for any zombie processes
            sleep(1);  
            bglist();
        } else if (strcmp(input[0], "bgkill") == 0) bgkill(strtol(input[1], NULL, 10));
        else if (strcmp(input[0], "bgstop") == 0) bgstop(strtol(input[1], NULL, 10));
        else if (strcmp(input[0], "bgstart") == 0) bgstart(strtol(input[1], NULL, 10));
        else if (strcmp(input[0], "pstat") == 0) pstat(strtol(input[1], NULL, 10));
        else errCorrGenUsage();                         //If specified command is none of the above, print a statement of proper usage
    }
    return 0;
}