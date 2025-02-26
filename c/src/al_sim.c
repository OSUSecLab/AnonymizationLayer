/******************************************************************************
Abstraction Layer
Author: Christopher Ellis
Implementation in C
******************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <zmq.h>
#include <stdatomic.h>
#include <signal.h>
#include "al_lib.h"

extern ALProfile profile_send_lookup;
extern ALProfile profile_encrypt;
extern ALProfile profile_recv_lookup;
extern ALProfile profile_decrypt;
extern ALProfile profile_simwork;
extern ALProfile profile_pseudo;

extern ALProfile profile_send_lookup_cpu;
extern ALProfile profile_encrypt_cpu;
extern ALProfile profile_recv_lookup_cpu;
extern ALProfile profile_decrypt_cpu;
extern ALProfile profile_simwork_cpu;
extern ALProfile profile_pseudo_cpu;

int total_recv_packet = 0;
int total_send_packet = 0;
int total_no_pair = 0;
int total_pseudo_gen = 0;

/* Run AL */
int main(int argc, char** argv)
{
    al_print("===================================\n");
    al_print(" Abstraction Layer Simulation Demo \n");
    al_print("===================================\n");

    Device* dev1 = NULL;
    Device* dev2 = NULL;
    Device* dev3 = NULL;
    macaddr_t macaddr1;
    macaddr_t macaddr2;
    macaddr_t macaddr3;
    int dev_one_id = 1;
    int dev_two_id = 2;
    int dev_three_id = 3;
    DevicePair* devpair1;
    DevicePair* devpair2;
    char dev1_key[DEVICEPAIR_KEY_SIZE];
    char dev2_key[DEVICEPAIR_KEY_SIZE];
    L2Frame frame = {0};
    L2Frame frame_copy = {0};
    L2Frame recvframe = {0};
    ALPacket packet = {0};
    ALPacket packet_copy = {0};
    ALPacket pseudopacket = {0};
    result_t result = SUCCESS;
    

    // Initialize random seed
    // srand(0);
    // srand(time(NULL));
    
    // Device details
    al_log("Creating Devices");
    result = Device_create_malloc(dev_one_id, &dev1);
    result = gen_macaddr_from_int(dev_one_id, &macaddr1);
    Device_assignaddress(dev1, macaddr1);

    result = Device_create_malloc(dev_two_id, &dev2);
    result = gen_macaddr_from_int(dev_two_id, &macaddr2);
    Device_assignaddress(dev2, macaddr2);

    result = Device_create_malloc(dev_three_id, &dev3);
    result = gen_macaddr_from_int(dev_three_id, &macaddr3);
    Device_assignaddress(dev3, macaddr3);

    // Helpful print.
    print_dev_all(dev1);    
    print_dev_all(dev2);  
    print_dev_all(dev3);  

    // Pair Devices - Just dev 1 and 2. 3 is another device in the wild.
    al_log("Pairing Devices");
    
    // Simulate receiving the device key.
    // gen_rnd_bytes_inplace(dev1_key, sizeof(dev1_key));
    // gen_rnd_bytes_inplace(dev2_key, sizeof(dev2_key));
    gen_key_from_ints(1, 2, dev1_key, sizeof(dev1_key));
    gen_key_from_ints(2, 1, dev2_key, sizeof(dev2_key));
    result = Device_createpairingrecord(dev1, dev2, dev1_key, dev2_key, &devpair1);
    result = Device_createpairingrecord(dev2, dev1, dev2_key, dev1_key, &devpair2);

    // Deprecate this in favor of single device pairing - more true to implementation.
    // Device_pairdevices(dev1, dev2);

    al_log("Device 1 pairs:\n");
    Device_printallpairs(dev1);

    // TODO move this to a print all paired devices function
    al_log("Device 2 pairs:\n");
    Device_printallpairs(dev2);

    al_log("Device 3 pairs:\n");
    Device_printallpairs(dev3);

    /* Send Data */
    // Get the first (only) pairing record from device 2 and send dummy data.
    #define NUM_PACKETS 20000
    for (int i = 0; i < NUM_PACKETS; i++)
    // while(1)
    {
    // Simulate receiving data from L2 with MAC address
    memcpy(frame.dst, macaddr2, sizeof(macaddr_t));
    memcpy(frame.src, macaddr1, sizeof(macaddr_t));
    gen_rnd_bytes_inplace(frame.payload, L2_MTU);

    // Copy for device 3
    
    al_log("Dev1 Generated L2 frame for Dev2:\n");
    print_l2frame_all(&frame, SMALL_MTU);
    print_space();

    al_log("Creating AL Packet:");
    Device_wrappacket(dev1, (uint8_t*)&frame, &packet);
    // Copy the packet for other dev since unwrapping modifies value.
    memcpy(&packet_copy, &packet, sizeof(ALPacket));
    
    // Print out packet
    print_alpacket_all(&packet, SMALL_MTU);
    print_space();

    // Determine a simulation packet time TODO
    al_log("Transmitting packet with delay X.\n");
    // Optionally sleep for x time to simulate delay.

    // Receive a packet and either create the frame or send a pseudo response
    al_log("Dev2: Received Packet from wire!");
    print_alpacket_all(&packet, SMALL_MTU);
    result = Device_unwrappacket(dev2, &packet, (uint8_t*)&recvframe);
    print_space();

    if (result != SUCCESS) 
    {   
        // COmment out to test no pseudo
        // al_log("Couldn't find pairing record, creating pseudoresponse.\n");
        if (rand() % 2 == 0) {
        Device_preparepseudo(dev2, &frame, &pseudopacket);
        
        al_log("Generated AL packet:");
        print_alpacket_all(&pseudopacket, SMALL_MTU);
        print_space();
        }
        // TODO and send over wire
    }
    else
    {
        al_log("Received L2 frame:");
        print_l2frame_all(&recvframe, SMALL_MTU);
        print_space();

        // TODO process time?
    }

    // Now simulate device 3 receiving the same packet as Dev2
    // Device 3 does not have pairing record.
    // al_log("Dev3: Received Packet from wire!");
    // print_alpacket_all(&packet_copy, SMALL_MTU);
    // print_space();

    // result = Device_unwrappacket(dev3, &packet_copy, (uint8_t*)&recvframe);
    // print_space();

    // if (result != SUCCESS) 
    // {
    //     al_log("Couldn't find pairing record, creating pseudoresponse.\n");
    //     Device_preparepseudo(dev3, &frame_copy, &pseudopacket);
        
    //     al_log("Generated AL packet:");
    //     print_alpacket_all(&pseudopacket, SMALL_MTU);
    //     print_space();

    //     // TODO and send over wire
    // }
    // else
    // {
    //     al_log("Received L2 frame:");
    //     print_l2frame_all(&recvframe, SMALL_MTU);
    //     print_space();

    //     // TODO process time?
    // }
        total_recv_packet++;
        total_send_packet++;
    }
    // End of Demo.


         // Print stats.
    al_log("Printing stats:");
    char results_buffer[5000] = {0};
    char *results_ptr = results_buffer;

    double total_processing_sec = total_recv_packet * ((SIM_IO_TIME+sizeof(L2_MTU)) / 1e6);
    double total_latency_sec = total_send_packet * TRANSMISSION_LATENCY / 1e6;
    double total_latency_baseline_sec = total_send_packet * TRANSMISSION_LATENCY_BASELINE / 1e6;

    results_ptr += sprintf(results_ptr, "{");
    results_ptr += sprintf(results_ptr, "\"dev_id\": %lu, ", dev1->id);
    results_ptr += sprintf(results_ptr, "\"l2_mtu\": %d, ", L2_MTU);
    results_ptr += sprintf(results_ptr, "\"num_sent\": %d, ", total_send_packet);
    results_ptr += sprintf(results_ptr, "\"num_recv\": %d, ", total_recv_packet);
    results_ptr += sprintf(results_ptr, "\"num_pseudo\": %d, ", total_pseudo_gen);
    results_ptr += sprintf(results_ptr, "\"num_nopair\": %d, ", total_no_pair);
    results_ptr += sprintf(results_ptr, "\"latency\": %f, ", total_latency_sec);
    results_ptr += sprintf(results_ptr, "\"latency_baseline\": %f, ", total_latency_baseline_sec);

    results_ptr += sprintf(results_ptr, "\"send_lookup\": %lf, ", profile_send_lookup.total);
    results_ptr += sprintf(results_ptr, "\"encryption\": %lf, ", profile_encrypt.total);
    results_ptr += sprintf(results_ptr, "\"recv_lookup\": %lf, ", profile_recv_lookup.total);
    results_ptr += sprintf(results_ptr, "\"decryption\": %lf, ", profile_decrypt.total);
    results_ptr += sprintf(results_ptr, "\"simwork\": %lf, ", profile_simwork.total);
    
    results_ptr += sprintf(results_ptr, "\"send_lookup_cpu\": %lf, ", profile_send_lookup_cpu.total);
    results_ptr += sprintf(results_ptr, "\"encryption_cpu\": %lf, ", profile_encrypt_cpu.total);
    results_ptr += sprintf(results_ptr, "\"recv_lookup_cpu\": %lf, ", profile_recv_lookup_cpu.total);
    results_ptr += sprintf(results_ptr, "\"decryption_cpu\": %lf, ", profile_decrypt_cpu.total);
    results_ptr += sprintf(results_ptr, "\"simwork_cpu\": %lf", profile_simwork_cpu.total);
    results_ptr += sprintf(results_ptr, "}");

    al_log("Results: (Also sent to stderr to capture)");
    printf("%s\n", results_buffer);
    fprintf(stderr, "%s\n", results_buffer);

    // Cleanup
    Device_delete(&dev1);
    Device_delete(&dev2);

    al_log("End of simulation.");

    // PROTOTYPING
    // int fd = fdopen("/tmp/al_fd");

}

