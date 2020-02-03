//all what I need
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <time.h>
#include <stdbool.h>

//semaphores declaration
sem_t* semMain;         //main semaphore - lock critical section: printing and variable's changes
sem_t* semMustLeave;    //if there is any waiting adult and he can now leave, semaphore lock other processes and adult leaves
sem_t* semAdult;        //semaphore which lock waiting adults
sem_t* semChild;        //semaphore which lock waiting children 
sem_t* semFinished;     //semaphore which ensure, that all processes finish at the end
sem_t* semWaitingGo;    //semaphore which stop waiting children if adult hasn't entered
sem_t* semStepIn;       //semaphore which ensure, that waiting children step in immediately

//shared variables declarations
int *printCounter = NULL;       //serial number of printed line
int *childCounter = NULL;       //number ofchildren in center
int *adultCounter = NULL;       //number of adults in center
int *childFinished = NULL;      //number of child processes which are waiting to finish
int *adultFinished = NULL;      //number of adult processes which are waiting to finish 
int *waitingChild = NULL;       //number of children waiting for enter
int *totalChild = NULL;         //number of child processes that already started
int *totalAdult = NULL;         //number of adult processes that already started
int *waitingAdult = NULL;       //flag - if there is some waiting adult

//shared variables Id's
int printCounterId = 0;
int childCounterId = 0;
int adultCounterId = 0;
int childFinishedId = 0;
int adultFinishedId = 0;
int waitingChildId = 0;
int totalChildId = 0;
int totalAdultId = 0;
int waitingAdultId = 0;

//file variable declaration
FILE *file = NULL;

//global variable-array of arguments
int arguments[6] = { 0 };

//Prototypes of functions
void child_action();
void adult_action();
void create_shared_memory();
void clean_shared_memory();
void create_semaphores();
void close_semaphores();
void close_program();
void terminate_program();

/***
*   Main function
*   Handling arguments
*   Creating processes
*   Calling modules
*/

int main(int argc, char* argv[]){

    /*catch signals*/
    signal(SIGTERM, terminate_program);
    signal(SIGINT, terminate_program);

    /*Open file for writing*/
    file = fopen("proj2.out", "w");
    if (file == NULL)
    {
        fprintf(stderr, "Opening file for writing failed!\n");
        return 2;
    }

    /*For correct printing*/
    setbuf(file, NULL);  
    setbuf(stderr, NULL);

    srand(time(0));   //Initialization of pseudorandom numbers generator

    /*Handling arguments*/
    if (argc != 7){
        fprintf (stderr,"Invalid number of arguments!\n");
        return 1;
    }

    char* bad_arg;      //pointer to bad string  

    /*bad number tests*/
    for (int i = 0; i < 6; i++){
        arguments[i] = (int) strtol(argv[i+1], &bad_arg, 10);

        if (strcmp(bad_arg, "")){
            fprintf (stderr,"Invalid arguments!\n");
            return 1;
        }
    }

    /*1,2 arguments tests*/
    for (int i = 0; i < 2; i++){
        if (arguments[i] <= 0){
            fprintf (stderr,"Invalid arguments!\n");
            return 1;
        }
    }

    /*3-6 arguments tests*/
    for (int i = 2; i < 6; i++){
        if (arguments[i] < 0 || arguments[i] > 5000){
            fprintf (stderr,"Invalid arguments!\n");
            return 1;
        }
        arguments[i]++;         //for valid randomize
    }

    /**************Handling arguments end************************************/

    /*Create semaphores and shared variables*/
    create_semaphores();
    create_shared_memory();
    

    /*Create auxiliary process that will create adult processes*/
    pid_t adultPid;
    pid_t adultCreatorPid = fork();

    if (adultCreatorPid == 0){  
        for (int i = 0; i < arguments[0]; i++){  
            usleep((rand() % arguments[2]) * 1000);
            adultPid = fork();

            if (adultPid == 0){
                adult_action();
                exit(0);
            }   

            /*if error occures, terminate*/
            if (adultPid < 0){
                fprintf(stderr,"Child create failed!\n");
                kill(adultCreatorPid, SIGTERM);
                terminate_program(0);
            }
        }

        /*other process, waiting*/
        if (adultPid > 0){
            while(wait(NULL) > 0);
        }
        exit(0);
    }

    /*if error occures, terminate*/
    if (adultCreatorPid < 0){
        fprintf(stderr,"Child create failed!\n");
        terminate_program(0);
    }

    /*Create auxiliary process that will create child processes*/
    pid_t childPid;
    pid_t childCreatorPid = fork();

    if (childCreatorPid == 0){   
        for (int i = 0; i < arguments[1]; i++){
            usleep((rand() % arguments[3]) * 1000); 
            childPid = fork();

            if (childPid == 0){
                child_action();
                exit(0);
            }   

            /*if error occures, terminate*/
            if (childPid < 0){
                fprintf(stderr,"Child create failed!\n");
                kill(childCreatorPid, SIGTERM);
                terminate_program(0);
            }
        }

        /*other process, waiting*/
        if (childPid > 0){
            while(wait(NULL) > 0);
        }
        exit(0);
    } 

    /*if error occures, terminate*/
    if (childCreatorPid < 0){
        fprintf(stderr,"Child create failed!\n");
        kill(adultCreatorPid, SIGTERM);
        terminate_program(0);
    }
    
    /*waiting for other processes*/
    if((adultCreatorPid > 0) && (childCreatorPid > 0)){
        while(wait(NULL) > 0); 
    }

    /*close file, semaphores, clean memory*/      
    close_program();    

    return 0;
}

/* Function simulating adult action */

void adult_action(){

    /*adult start*/
    sem_wait(semMain);
    (*printCounter)++;
    (*totalAdult)++;
    int adult = *totalAdult;
    fprintf(file,"%d\t: A %d\t: started\n", *printCounter, adult);
    sem_post(semMain);

    /*adult enter*/
    sem_wait(semMain);
    (*printCounter)++;
    fprintf(file,"%d\t: A %d\t: enter\n", *printCounter, adult);
    (*adultCounter)++;

    /*children can go*/
    for (int i = 0; i < 3; i++){
        sem_post(semChild);
    }

    /*adult can  go*/
    if ((*childCounter <= 3 * ((*adultCounter)-1)) && *waitingAdult){
        sem_post(semAdult);
        sem_wait(semMustLeave); 
    }

    /*waiting children can go*/
    else{   
        if (*waitingChild){
            int help;
            if (*waitingChild > 3){
                help = 3;
            }
            else{
                help = *waitingChild;
            }
            for (int i = 0; i < help; i++){
                sem_post(semWaitingGo);
                sem_wait(semStepIn);   
            }  
        }
    }
    sem_post(semMain);

    /*simulating action*/
    usleep((rand() % arguments[4]) * 1000);

    /*leaving*/
    sem_wait(semMain);
    (*printCounter)++;
    fprintf(file,"%d\t: A %d\t: trying to leave\n", *printCounter, adult);
    
    /*leave instantly if statement is true*/
    if (*childCounter <= 3 * (*adultCounter - 1)){
        (*printCounter)++;
        fprintf(file,"%d\t: A %d\t: leave\n", *printCounter, adult);
        (*adultCounter)--;

        for (int i = 0; i < 3; i++){
            sem_wait(semChild);
        }
        (*adultFinished)++;
        sem_post(semMain);
    }

    /*waiting for leaving*/
    else{
        (*printCounter)++;
        fprintf(file,"%d\t: A %d\t: waiting : %d : %d\n", *printCounter, adult, *adultCounter, *childCounter);
        (*waitingAdult)++;
        sem_post(semMain);

        /*waiting on semaphore*/
        sem_wait(semAdult);
        (*printCounter)++;
        fprintf(file,"%d\t: A %d\t: leave\n", *printCounter, adult);
        (*adultCounter)--;
        (*waitingAdult)--;

        for (int i = 0; i < 3; i++){
            sem_wait(semChild);
        }
        (*adultFinished)++;
        sem_post(semMustLeave);
    }
    sem_wait(semMain);
    
    /*if there isn't any adults, let children go*/
    if (*adultFinished == arguments[0]){
        int waiting = *waitingChild;
        for (int i = 0; i < waiting; i++){
            sem_post(semWaitingGo);
        }    

        for (int i = 0; i < waiting; i++){
            sem_post(semChild);
            sem_wait(semStepIn);
        }    
    }  
    sem_post(semMain);

    /*check, if all processes are waiting to finish*/
    sem_wait(semMain);        
    if (((*adultFinished) == arguments[0]) && ((*childFinished) == arguments[1])){
        (*childFinished)++;                                       //semFinished won't be incremented in child's procces
        
        for (int i = 0; i < (arguments[0] + arguments[1]); i++){
            sem_post(semFinished);
        }
    }
    sem_post(semMain);

    /*waiting on 'finish' semaphore*/
    sem_wait(semFinished);
    sem_wait(semMain);
    (*printCounter)++;
    fprintf(file,"%d\t: A %d\t: finished\n", *printCounter, adult);
    sem_post(semMain);
}

/* Function simulating child action */

void child_action(){

    /*child start*/
    sem_wait(semMain);
    (*printCounter)++;
    (*totalChild)++;
    int child = *totalChild;
    fprintf(file,"%d\t: C %d\t: started\n", *printCounter, child);
    sem_post(semMain);

    /*child try to enter*/
    sem_wait(semMain);  
    if (((*childCounter)+1 <= 3 * (*adultCounter)) || (*adultCounter == 0 && ((*adultFinished) == arguments[0]))){
        if ((*adultFinished) != arguments[0]){
            sem_wait(semChild); 
        }

        (*childCounter)++;
        (*printCounter)++;  
        fprintf(file,"%d\t: C %d\t: enter\n", *printCounter, child);
        sem_post(semMain);
    }

    /*waiting if statement is false*/
    else{
        (*printCounter)++;
        fprintf(file,"%d\t: C %d\t: waiting : %d : %d\n", *printCounter, child, *adultCounter, *childCounter);
        (*waitingChild)++;
        sem_post(semMain);

        sem_wait(semWaitingGo);
        sem_wait(semChild);
       
        (*childCounter)++;
        (*printCounter)++;
        fprintf(file,"%d\t: C %d\t: enter\n", *printCounter, child);
        (*waitingChild)--;
        sem_post(semStepIn);
    }

    /*simulating action*/
    usleep((rand() % arguments[5]) * 1000); 

    /*leaving*/
    sem_wait(semMain);    
    (*printCounter)++;
    fprintf(file,"%d\t: C %d\t: trying to leave\n", *printCounter, child);
    (*printCounter)++;
    fprintf(file,"%d\t: C %d\t: leave\n", *printCounter, child);
    (*childCounter)--;
    sem_post(semChild);
    
    /*if there are adults who can now leave*/
    if ((*childCounter <= 3 * ((*adultCounter)-1)) && *waitingAdult){
        sem_post(semAdult);
        sem_wait(semMustLeave);
    }
    (*childFinished)++;
    sem_post(semMain);

    /*check, if all processes are waiting to finish*/
    sem_wait(semMain);
    if (((*adultFinished) == arguments[0]) && ((*childFinished) == arguments[1])){
        (*childFinished)++;                 //semFinished won't be incremented in adult's procces
        for (int i = 0; i < (arguments[0] + arguments[1]); i++){
            sem_post(semFinished);
        }
    }
    sem_post(semMain);

    /*waiting on 'finish' semaphore*/
    sem_wait(semFinished);
    sem_wait(semMain);
    (*printCounter)++;
    fprintf(file,"%d\t: C %d\t: finished\n", *printCounter, child);
    sem_post(semMain);
}

/* Function allocs shared memory */

void create_shared_memory(){

    /*get Id's*/
    printCounterId = shmget(IPC_PRIVATE, sizeof (int), IPC_CREAT | 0666);
    childCounterId = shmget(IPC_PRIVATE, sizeof (int), IPC_CREAT | 0666);
    adultCounterId = shmget(IPC_PRIVATE, sizeof (int), IPC_CREAT | 0666);
    waitingChildId = shmget(IPC_PRIVATE, sizeof (int), IPC_CREAT | 0666);
    childFinishedId = shmget(IPC_PRIVATE, sizeof (int), IPC_CREAT | 0666);
    adultFinishedId = shmget(IPC_PRIVATE, sizeof (int), IPC_CREAT | 0666);
    totalChildId = shmget(IPC_PRIVATE, sizeof (int), IPC_CREAT | 0666);
    totalAdultId = shmget(IPC_PRIVATE, sizeof (int), IPC_CREAT | 0666);
    waitingAdultId = shmget(IPC_PRIVATE, sizeof (int), IPC_CREAT | 0666);
 
    /*if error ocurres, terminate*/
    if (printCounterId == -1 || childCounterId == -1 || adultCounterId == -1 || waitingChildId == -1 || 
        childFinishedId == -1 || totalChildId == -1 || totalAdultId == -1 || waitingAdultId == -1 || adultFinishedId == -1){ 
        fprintf(stderr,"Nastala chyba pri vytvarani zdielanej pamate!\n");
        close_program();
        exit(2);
    }

    /*get pointers*/
    printCounter = (int *) shmat(printCounterId, NULL, 0);
    childCounter = (int *) shmat(childCounterId, NULL, 0);
    adultCounter = (int *) shmat(adultCounterId, NULL, 0);
    waitingChild = (int *) shmat(waitingChildId, NULL, 0);
    childFinished = (int *) shmat(childFinishedId, NULL, 0);
    adultFinished = (int *) shmat(adultFinishedId, NULL, 0);
    totalChild = (int *) shmat(totalChildId, NULL, 0);
    totalAdult = (int *) shmat(totalAdultId, NULL, 0);
    waitingAdult = (int *) shmat(waitingAdultId, NULL, 0);
    
    /*if error ocurres, terminate*/
    if (printCounter == NULL || childCounter == NULL || adultCounter == NULL || waitingChild == NULL || 
        childFinished == NULL || totalChild == NULL || totalAdult == NULL || waitingAdult == NULL || adultFinished == NULL){ 
        fprintf(stderr,"Nastala chyba pri vytvarani zdielanej pamate!\n");
        close_program();
        exit(2);
    } 

    /*initialize*/
    *printCounter = 0;
    *childCounter = 0;
    *adultCounter = 0;
    *waitingChild = 0;
    *childFinished = 0;
    *adultFinished = 0;
    *totalChild = 0;
    *totalAdult = 0;
    *waitingAdult = 0;
}

/* This function cleans shared memory */

void clean_shared_memory(){
    shmctl(printCounterId, IPC_RMID, NULL);
    shmctl(childCounterId, IPC_RMID, NULL);
    shmctl(adultCounterId, IPC_RMID, NULL);
    shmctl(waitingChildId, IPC_RMID, NULL);
    shmctl(childFinishedId, IPC_RMID, NULL);
    shmctl(adultFinishedId, IPC_RMID, NULL);
    shmctl(totalChildId, IPC_RMID, NULL);
    shmctl(totalAdultId, IPC_RMID, NULL);
    shmctl(waitingAdultId, IPC_RMID, NULL);
}

/* This function creates semaphores */

void create_semaphores(){
    semMain = sem_open("/xnerec00semMain", O_CREAT | O_EXCL, 0666, 1);
    semMustLeave = sem_open("/xnerec00semMustLeave", O_CREAT | O_EXCL, 0666, 0);
    semAdult = sem_open("/xnerec00semAdult", O_CREAT | O_EXCL, 0666, 0);
    semChild = sem_open("/xnerec00semChild", O_CREAT | O_EXCL, 0666, 0);
    semFinished = sem_open("/xnerec00semFinished", O_CREAT | O_EXCL, 0666, 0);
    semWaitingGo = sem_open("/xnerec00semWaitingGo", O_CREAT | O_EXCL, 0666, 0);
    semStepIn = sem_open("/xnerec00semStepIn", O_CREAT | O_EXCL, 0666, 0);

    if(semMain == SEM_FAILED || semAdult == SEM_FAILED || semChild == SEM_FAILED || semMustLeave == SEM_FAILED 
        || semFinished == SEM_FAILED || semWaitingGo == SEM_FAILED || semStepIn == SEM_FAILED)
    {
        fprintf(stderr,"Nastala chyba pri vytvarani semaforov!\n");
        close_program();
        exit(2);
    }
}

/* This function close and unlink all semaphores */

void close_semaphores(){

    /*close*/
    sem_close(semMain);
    sem_close(semMustLeave);
    sem_close(semAdult);
    sem_close(semChild);
    sem_close(semFinished);
    sem_close(semWaitingGo);
    sem_close(semStepIn);


    /*unlink*/
    sem_unlink("/xnerec00semMain");
    sem_unlink("/xnerec00semMustLeave");
    sem_unlink("/xnerec00semAdult");
    sem_unlink("/xnerec00semChild");
    sem_unlink("/xnerec00semFinished");
    sem_unlink("/xnerec00semWaitingGo");
    sem_unlink("/xnerec00semStepIn");
}

/*Function which close writing file, clean memory and close semaphores*/

void close_program(){
    fclose(file);           //close file
    clean_shared_memory();  //clean memory
    close_semaphores();     //close and unlink semaphores
}

/*Fuction which is called if error occured*/

void terminate_program(int sig_hand){
    (void)sig_hand;             //only to use sig_hand variable
    kill(getpid(), SIGTERM);    //kill process
    close_program();            //clean memory and close
    exit(2);                    //error's return code
}