/******************************************************************************
Abstraction Layer - Multiple Devices
Author: Christopher Ellis
Implementation in C
******************************************************************************/

#include "al_lib.h"
#include <zmq.h>
#include <signal.h>
#include "czmq.h"


#define NUM_DEVICES 5
#define ARG_LENGTH 50

pid_t child_pids[NUM_DEVICES];

// static void listener_thread(void *args, void *pipe)
// {
    //  Print everything that arrives on pipe
    // while (true) {
        // zframe_t *frame = zframe_recv (pipe);
        // if (!frame)
            // break;              //  Interrupted
        // zframe_print (frame, NULL);
        // zframe_destroy (&frame);
    // }
// }



// Signal handler for SIGINT (Ctrl+C)
void handle_signal(int signal) {
    int i = 0;
    if (signal == SIGINT) {
        printf("\nSIGINT received by parent. Terminating child process...\n");
        for (i = 0; i < NUM_DEVICES; i++)
        {
            kill(child_pids[i], SIGINT);
        }
    }
}

/* Run AL */
int main(int argc, char** argv)
{
    printf("======================================================\n");
    printf(" Abstraction Layer Simulation Demo - Multiple Devices \n");
    printf("======================================================\n");

    result_t result = SUCCESS;
    int i = 0;
    pid_t pid;

    // Initialize random seed
    srand(0);

    #define DEV_FILEPATH "/bin/al_sim_device" // relative to CWD
    // #define DEV_FILEPATH "/al_sim_device" // relative to CWD
    int num_devices = NUM_DEVICES;
    int arg_length = ARG_LENGTH;

    // Manuall create args - regular inline code profiling
    // char *dev_args[][ARG_LENGTH] = {
    //     {DEV_FILEPATH, "1", "2", "3", "4", "5", "6", NULL},
    //     {DEV_FILEPATH, "2", "1", "2", "4" , "5", "6", NULL},
    //     {DEV_FILEPATH, "3", "1", "2", "4" , "5", "6", NULL},
    //     {DEV_FILEPATH, "4", "1", "2", "4" , "5", "6", NULL},
    //     {DEV_FILEPATH, "5", "1", "2", "4" , "5", "6", NULL},
    //     {DEV_FILEPATH, "6", "1", "2", "4" , "5", "6", NULL},
    //     // These will not have pairing records with any other.
    //     {DEV_FILEPATH, "7", "1", "2", "4" , "5", "6", NULL},
    //     {DEV_FILEPATH, "8", "1", "2", "4" , "5", "6", NULL},
    //     {DEV_FILEPATH, "9", "1", "2", "4" , "5", "6", NULL},
    //     {DEV_FILEPATH, "10", "1", "2", "4" , "5", "6", NULL},
    // };
    
    // This grouping has 2 devices successfully paired and communicating with each other.
    char *dev_args[][ARG_LENGTH] = {
        {DEV_FILEPATH, "1", "2", "30", "40", "50", "60", NULL},
        {DEV_FILEPATH, "2", "1", "20", "40", "50", "60", NULL},
        {DEV_FILEPATH, "3", "4", "20", "40", "50", "60", NULL},
        {DEV_FILEPATH, "4", "3", "20", "40", "50", "60", NULL},
        {DEV_FILEPATH, "5", "6", "20", "40", "50", "60", NULL},
        {DEV_FILEPATH, "6", "5", "20", "40", "50", "60", NULL},
        // These will not have pairing records with any other.
        {DEV_FILEPATH, "7", "1", "2", "4", "5", "6", NULL},
        {DEV_FILEPATH, "8", "1", "2", "4", "5", "6", NULL},
        {DEV_FILEPATH, "9", "1", "2", "4", "5", "6", NULL},
        {DEV_FILEPATH, "10", "1", "2", "4", "5", "6", NULL},
    };

    // String for path of singluar device.
    // Make sure the file is in the same directory.
    char cwd[100];
    char* device_exec_filepath = getcwd(cwd, sizeof(cwd));
    strncat(device_exec_filepath, DEV_FILEPATH, sizeof(DEV_FILEPATH));
    
    printf("Opening %s\n", cwd);

    char gmon_path[100];
    char new_gmon[100];
    snprintf(gmon_path, sizeof(unsigned long) + 19, "GMON_OUT_PREFIX=al_%lu", (unsigned long)time(NULL));

    unsetenv("GMON_OUT_PREFIX"); 
    putenv("GMON_OUT_PREFIX=dev_multi");

    al_log("Starting singular device processes");
    for (i = 0; i < num_devices; i++)
    {
        child_pids[i] = fork();
        // printf("Started process: %d\n", child_pids[i]);
        if (child_pids[i] == -1)
        {
            al_log("Something terrible happened! Exiting.");
            exit(FAIL);
        }

        // Start the new device or continue on.
        if (child_pids[i] == 0) // New process
        {
            execv(device_exec_filepath, dev_args[i]);
        }

    }

    // Cleanup
    int status;
    for (i = 0; i < NUM_DEVICES; i++)
    {
        waitpid(child_pids[i], &status, 0);
    }

    // End.
    al_log("End of multi simulation.");
    return result;

}

