/******************************************************************************
 Abstraction Layer - Single Device Upper Layer
 Author: Christopher Ellis
******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <zmq.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include "al_lib.h"

#define DATA_SIZE L2_MTU
#define NUM_SEND 1000

// Structure to hold data for each thread
typedef struct {
    int src_device_id;
    int dst_device_id;
} thread_data_t;


// Thread function that takes void* argument
void* send_upper_layer(void* arg) {

    macaddr_t src_macaddr;
    macaddr_t dst_macaddr;
    L2Frame frame = {0};
    int i = 0;
    int num_send = NUM_SEND;
    result_t result = SUCCESS;

    // Cast the void* argument back to a pointer to thread_data_t
    thread_data_t *data = (thread_data_t*)arg;
    printf("Starting thread for device ID: %d\n", data->src_device_id);

    // Create connection address strings
    uint32_t single_dev_recv_port = AL_DOMAIN_UPPERLAYER_PORT_START + data->src_device_id;
    char single_dev_address[sizeof(AL_DOMAIN_UPPERLAYER) + 1 + 5 + 1] = {0}; // "@" + ":" + max port length of 5 + null.
    snprintf(single_dev_address, sizeof(single_dev_address), "%s:%d", AL_DOMAIN_UPPERLAYER, single_dev_recv_port); // This format is per czmq


    // Open to simulate receiving from upper layer
    printf("Connecting to %s\n", single_dev_address);
    void *context = zmq_ctx_new();
    void *dev_input = zmq_socket(context, ZMQ_PUSH);
    zmq_connect(dev_input, single_dev_address);
    if (dev_input == NULL)
    {
        al_log("Opening socket failed.");
    }

    // Create macaddrs
    gen_macaddr_from_int(data->src_device_id, &src_macaddr);
    gen_macaddr_from_int(data->dst_device_id, &dst_macaddr);

    // Set Macaddrs
    memcpy(frame.dst, dst_macaddr, sizeof(macaddr_t));
    memcpy(frame.src, src_macaddr, sizeof(macaddr_t));

    // Send data
    for (i = 0; i < num_send; i++)
    {
        // Create data
        gen_rnd_bytes_inplace(frame.payload, L2_MTU); // Random

        al_log("Created frame:");
        print_l2frame_all(&frame, SMALL_MTU);

        // Send data over socket
        if (dev_input == NULL)
        {
            al_log("Opening socket failed.");
        }
        
        printf("Sending packet %d.\n", i);
        zmq_send(dev_input, &frame, sizeof(L2Frame), 0);
        
        print_space();

        // Processing time. A simulation time for there and back/Round Trip.
        usleep(20000);

    }

    // Cleanup
    zmq_close(dev_input);
    zmq_ctx_destroy(context);
    al_log("End of sending data, exiting thread.");
    pthread_exit(NULL);

}


/******************************************************************************
 Main
******************************************************************************/

int main(int argc, int** argv)
{
    printf("===============================================================\n");
    printf(" Abstraction Layer Simulation Demo - Multi Device Upper Layer \n");
    printf("===============================================================\n");

    int num_devices = 10;
    pthread_t threads[num_devices];              // Array to hold thread IDs
    thread_data_t thread_data[num_devices];      // Array to hold data for each thread
    int i = 0;
    int num_send = 0;
    result_t result = SUCCESS;

    // Seed random.
    // srand(0); // Not random.
    srand((unsigned int)time(NULL)); // Random!

    // Define devices and pairings.
    // Dev 1 and 2
    thread_data[0].src_device_id = 1;
    thread_data[0].dst_device_id = 2;
    thread_data[1].src_device_id = 2;
    thread_data[1].dst_device_id = 1;
    // Dev 3 and 4
    thread_data[2].src_device_id = 3;
    thread_data[2].dst_device_id = 4;
    thread_data[3].src_device_id = 4;
    thread_data[3].dst_device_id = 3;
    // Dev 5 and 6
    thread_data[4].src_device_id = 5;
    thread_data[4].dst_device_id = 6;
    thread_data[5].src_device_id = 6;
    thread_data[5].dst_device_id = 5;

    for (i = 6; i < num_devices; i++)
    {
        thread_data[i].src_device_id = i+1;
        thread_data[i].dst_device_id = i+2;
    }
    
    // Create multiple threads and pass different data to each one
    for (int i = 0; i < num_devices; i++) {
        
        // Create a new thread and pass the address of the thread-specific data
        result = pthread_create(&threads[i], NULL, send_upper_layer, (void*)&thread_data[i]);
        if (result) {
            printf("Error: Unable to create thread %d, error code: %d\n", i + 1, result);
            exit(FAIL);
        }

    }
        
    // Cleanup
    for (int i = 0; i < num_devices; i++) {
        pthread_join(threads[i], NULL);
    }

    al_log("Upper layer multi finished.");
    return result;

}
