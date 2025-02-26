/******************************************************************************
 Abstraction Layer - Single Device Upper Layer
 Author: Christopher Ellis
******************************************************************************/

#include <pthread.h>
#include <time.h>
#include "al_lib.h"
#include "czmq.h"


/******************************************************************************
 Main
******************************************************************************/

int main(int argc, int** argv)
{
    printf("===============================================================\n");
    printf(" Abstraction Layer Simulation Demo - Single Device Upper Layer \n");
    printf("===============================================================\n");

    int src_device_id;
    int dst_device_id;
    macaddr_t src_macaddr;
    macaddr_t dst_macaddr;
    L2Frame frame = {0};
    int i = 0;
    int j = 0;
    int num_send = 0;
    int num_repeat = 0;
    result_t result = SUCCESS;

    // Seed random.
    // srand(0); // Not random.
    srand((unsigned int)time(NULL)); // Random!

    // First argument: Which device ID to send to. Programatticaly create the port.
    if (argc < 4)
    {
        al_log("Needs atleast 3 arg: # packets to send, source device_id, and receiving device_id");
        exit(FAIL);
    }
    num_send = atoi((char*)(argv[1]));
    src_device_id = atoi((char*)(argv[2]));
    dst_device_id = atoi((char*)(argv[3]));
    if (argc == 5) num_repeat = atoi((char*)(argv[4]));

    // Create connection address strings
    uint32_t single_dev_recv_port = AL_DOMAIN_UPPERLAYER_PORT_START + src_device_id;
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
    gen_macaddr_from_int(src_device_id, &src_macaddr); 
    gen_macaddr_from_int(dst_device_id, &dst_macaddr);
    memcpy(frame.src, src_macaddr, sizeof(macaddr_t));
    memcpy(frame.dst, dst_macaddr, sizeof(macaddr_t));

    // Send data
    for (i = 0; i < num_send; i++)
    {
        // Create data
        #define NUM_DEVICE 100
        #define START_DEV_OFFSET 3 // Use this value to be the dev_id of the receiving device + 1. So dev 2, set to 3.
        // int rand_id = (rand() % NUM_DEVICE) + START_DEV_OFFSET;
        // gen_macaddr_from_int(rand_id, &src_macaddr); // we can ranomdize the source addr here to test the lookup time.
        // memcpy(frame.src, src_macaddr, sizeof(macaddr_t));
        
        gen_rnd_bytes_inplace(frame.payload, L2_MTU);

        al_log("Created frame:");
        print_l2frame_all(&frame, SMALL_MTU);

        // Send data over socket
        if (dev_input == NULL)
        {
            al_log("Opening socket failed.");
        }
        
        printf("Sending packet %d.\n", i);
        zmq_send(dev_input, &frame, sizeof(L2Frame), 0);

        // Repeat?
        for (j = 1; j < num_repeat; j++){
            zmq_send(dev_input, &frame, sizeof(L2Frame), 0);
        }
        
        print_space();

        usleep(1);

    }

    al_log("End of sending data.");

    // Free resources
    zmq_close(dev_input);
    zmq_ctx_destroy(context);

    // End of Demo.
    al_log("Done.");
    // return result;
    return SUCCESS;
}
