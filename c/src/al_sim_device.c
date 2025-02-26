/******************************************************************************
 Abstraction Layer - Single Device
 Author: Christopher Ellis
******************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <zmq.h>
#include <stdatomic.h>
#include <signal.h>
#include "al_lib.h"
// #include "czmq.h"

// Global device 
Device* dev = NULL;
pthread_mutex_t dev_mutex;
volatile sig_atomic_t stop = 0;

typedef uint8_t byte;

// Profile globals
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

#define TOTAL_TO_END 10000

#define USE_AL 1
#define USE_PROFILING 1


/******************************************************************************
 Simulation Functions
******************************************************************************/

void* sim_send_data_thread()
{
    result_t result = SUCCESS;
    L2Frame frame = {0};
    L2Frame frame_rnd;
    ALPacket packet = {0};
    macaddr_t macaddr;
    uint32_t single_dev_recv_port = AL_DOMAIN_UPPERLAYER_PORT_START + dev->id;
    char upperlayer_address_in[sizeof(AL_DOMAIN_UPPERLAYER) + 2 + 5 + 1] = {0}; // ">" + ":" + max port length of 5 + null.
    char broadcast_address_out[sizeof(AL_DOMAIN_CONNECT) + 1 + 5 + 1] = {0}; //  + ":" + max port length of 5 + null.
    size_t inc_size;

    // Latency for transmission
    double latency = BLE_TX_TIME_US(sizeof(ALPacket), BLE_1M_PHY);

    al_log("SEND: Sending thread started.");

    // Create ZMQ context.
    void *context = zmq_ctx_new();

    // Create connection address strings.
    snprintf(upperlayer_address_in, sizeof(upperlayer_address_in), "%s:%d", AL_DOMAIN_BIND, single_dev_recv_port); // This format is per czmq
    al_log("SEND: Connecting to receive data at upper layer from %s\n", upperlayer_address_in);
    
    snprintf(broadcast_address_out, sizeof(broadcast_address_out), "%s:%d", AL_DOMAIN_CONNECT, AL_PROXY_PORT_IN);
    al_log("SEND: Connecting to proxy to publish at: %s\n", broadcast_address_out);

    // Create publisher socket.
    void *pub_sock = zmq_socket(context, ZMQ_PUB);
    zmq_connect(pub_sock, broadcast_address_out);
    if (pub_sock == NULL)
    {
        al_log_error("Unable to connect to broadcast address.");
        zmq_strerror(errno);
    }

    // Open "upper layer" socket to simulate receiving data from layer above AL (L2 frame). 
    void *upperlayer_sock_in = zmq_socket(context, ZMQ_PULL);
    zmq_bind(upperlayer_sock_in, upperlayer_address_in);
    usleep(1);
    if (upperlayer_sock_in == NULL)
    {
        al_log_error("Unable to bind to upperlayer address.");
        zmq_strerror(errno);
    }

    al_log("SEND: Ready to receive data from upper layer.");

    int bytes_received = 0;
    // Start recv/send loop.
    while (!stop)
    {
        
        bytes_received = zmq_recv(upperlayer_sock_in, &frame, sizeof(L2Frame), ZMQ_DONTWAIT);
        if (bytes_received == NO_DATA)
        {
            usleep(1);
            continue;
        }

        // DEV ONLY - Or, for in-process random creation of packets. Leave for now.
        // result = gen_macaddr_from_int(2, &macaddr);
        // memcpy(frame_rnd.dst, macaddr, sizeof(macaddr_t));
        // memcpy(frame_rnd.src, dev->macaddr, sizeof(macaddr_t));
        // gen_rnd_bytes_inplace(frame_rnd.payload, L2_MTU);
        // frame = &frame_rnd;

        al_log_v("SEND: Received L2 Frame:");
        // VERBOSE_CALL(print_l2frame_all, &frame, SMALL_MTU);
        print_l2frame_all(&frame, 0);
        VERBOSE_CALL(print_space);

        #ifdef USE_AL
        al_log_v("SEND: Creating AL Packet:");

        // Wrap packet
        result = Device_wrappacket(dev, (uint8_t*)&frame, &packet);
        if (result != SUCCESS)
        {
            al_log_v("Wrapping packet failed.");
            // goto CLEANUP;
            continue;
        }
        
        // Print packet
        print_alpacket_all(&packet, 0);
        VERBOSE_CALL(print_space);

        al_log_v("SEND: Transmitting packet with delay %f", TRANSMISSION_LATENCY);
        VERBOSE_CALL(print_space);

        result = zmq_send(pub_sock, (byte*)&packet, sizeof(ALPacket), 0);
        // Log which address we sent to so we do not receive it on the broadcast.
        result = Device_set_lastsent(dev, packet.tkey);
        #else
        
        result = zmq_send(pub_sock, (byte*)&frame, sizeof(L2Frame), 0);

        #endif

        // Simulate latency for BLE.
        usleep(TRANSMISSION_LATENCY);

        // Track number of sent packets;
        total_send_packet++;
        if (total_send_packet % 100 == 0) al_log("SEND: Sent %d packets.", total_send_packet);

        if (TOTAL_TO_END && total_send_packet == TOTAL_TO_END)
        {
            stop = 1;
            goto CLEANUP;
        }

    }

    CLEANUP:
    zmq_close(pub_sock);
    usleep(10);
    zmq_close(upperlayer_sock_in);
    usleep(10);
    al_log("SEND: Thread exiting.");
    pthread_exit(NULL);

}


void* sim_recv_data_thread()
{
    result_t result = SUCCESS;
    L2Frame frame = {0};
    ALPacket packet = {0};
    char *z_frame;
    int count = 0;

    al_log("RECV: Receiving thread started.");

    // Create ZMQ context.
    void* context = zmq_ctx_new();

    // Open subcriber socket to receive broadcasted messages.
    char broadcast_address_in[sizeof(AL_DOMAIN_CONNECT) + 1 + 5 + 1] = {0}; //  + ":" + max port length of 5 + null.
    snprintf(broadcast_address_in, sizeof(broadcast_address_in), "%s:%d", AL_DOMAIN_CONNECT, AL_PROXY_PORT_OUT);
    al_log("RECV: Connecting to proxy as subscriber with %s\n.", broadcast_address_in);
    void* sub_sock = zmq_socket(context, ZMQ_SUB);
    zmq_connect(sub_sock, broadcast_address_in);
    zmq_setsockopt(sub_sock, ZMQ_SUBSCRIBE, "", 0); // Subscribes to all/any topics

    // Create publisher socket for pseudo response.
    char broadcast_address_out[sizeof(AL_DOMAIN_CONNECT) + 1 + 5 + 1] = {0}; //  + ":" + max port length of 5 + null.
    snprintf(broadcast_address_out, sizeof(broadcast_address_out), "%s:%d", AL_DOMAIN_CONNECT, AL_PROXY_PORT_IN);
    al_log("RECV: Connecting to proxy as publisher at: %s\n", broadcast_address_out);
    void *pub_sock = zmq_socket(context, ZMQ_PUB);
    zmq_connect(pub_sock, broadcast_address_out);
    if (pub_sock == NULL)
    {
        al_log("Unable to connect to broadcast address.");
        zmq_strerror(errno);
    }

    // Start recv thread.
    al_log("RECV: Ready to receive packets from broadcast.");
    int bytes_received;
    while (!stop) 
    {
        #ifdef USE_AL

        // bytes_received = zmq_recv(sub_sock, &packet, sizeof(ALPacket), 0);
        bytes_received = zmq_recv(sub_sock, &packet, sizeof(ALPacket), ZMQ_DONTWAIT);
        if (bytes_received == NO_DATA)
        {
            usleep(1);
            continue;
        }
        
        // Short circuit if this was sent by this device.
        if (Device_check_lastsent(dev, packet.tkey) == TRUE)
        {
            // Disregard packet.
            // printf("LAST SENT\n");
            continue;
        }

        al_log_v("RECV: Received Packet from wire!");
        // VERBOSE_CALL(print_alpacket_all, &packet, SMALL_MTU);
        print_alpacket_all(&packet, 0);
        VERBOSE_CALL(print_space);

        // Receive a packet and either create the frame or send a pseudo response
        result = Device_unwrappacket(dev, &packet, (uint8_t*)&frame);

        if (result == NO_PAIR) 
        {
            if (rand() % 5 == 0)
            {
                al_log_v("Couldn't find pairing record, creating pseudoresponse.\n");
                Device_preparepseudo(dev, &frame, &packet);
                VERBOSE_CALL(print_alpacket_all, &packet, SMALL_MTU);
                result = zmq_send(pub_sock, (byte*)&packet, sizeof(ALPacket), 0);
                // NOTE: Need to properly handle result in production.
                result = Device_set_lastsent(dev, packet.tkey);
                // NOTE: Need to properly handle result in production.
                total_pseudo_gen++;
            }
            else
            {
                al_log_v("Couldn't find pairing record, not creating pseudoresponse.\n");
            }

            total_no_pair++;
        }
        else
        if (result == FAIL)
        {
            al_log_error("Unwrapping packet failed for some general reason.");
        }
        else
        if (result == SUCCESS)
        {
            al_log_v("Found pairing record and unwrapped AL packet, containing L2 frame:");
            // VERBOSE_CALL(print_l2frame_all, &frame, SMALL_MTU);
            print_l2frame_all(&frame, 0);
            VERBOSE_CALL(print_space);
        }

        #else
        bytes_received = zmq_recv(sub_sock, &frame, sizeof(L2Frame), ZMQ_DONTWAIT);
        if (bytes_received == NO_DATA)
        {
            usleep(1);
            continue;
        }
        al_log_v("Received L2Frame:");
        VERBOSE_CALL(print_l2frame_all, &frame, SMALL_MTU);
        VERBOSE_CALL(print_space);
        #endif

        // Simulate processing of the data. i.e. sending up layers, writing to disk, computing on bare metal, etc.
        START_CLOCK_CPU(profile_simwork_cpu);
        START_CLOCK(profile_simwork);
        simulate_floating_point_work((uint8_t*)&frame.payload, sizeof(frame.payload));
        usleep(SIM_IO_TIME * sizeof(frame.payload)); // microseconds, relative to payload size for io
        STOP_CLOCK_CPU(profile_simwork_cpu);
        STOP_CLOCK(profile_simwork);

        total_recv_packet++;
        if (total_recv_packet % 100 == 0)
        {
            al_log("Received %d packets.", total_recv_packet);
        }

        if (TOTAL_TO_END && total_recv_packet == TOTAL_TO_END)
        {
            stop = 1;
            goto CLEANUP;
        }

    }

    // Cleanup
    CLEANUP:
    zmq_close(sub_sock);
    usleep(10);
    al_log("RECV: Thread exiting.");
    pthread_exit(NULL);

}


// Gracefully end
void handle_signal(int signal) {
    if (signal == SIGINT) {
        al_log("SIGINT received. Stopping threads...");
        stop = 1;  // Set the stop flag to signal all threads to exit
    }
}



/******************************************************************************
 Main
******************************************************************************/

int main(int argc, char** argv)
{
    printf("===================================================\n");
    printf(" Abstraction Layer Simulation Demo - Single Device \n");
    printf("===================================================\n");

    u_int64_t device_id;
    macaddr_t macaddr;
    DevicePair* devpair = NULL;
    u_int64_t num_devpairs = 0;
    pthread_t send_thread;
    pthread_t recv_thread;
    int i = 0;
    result_t result = SUCCESS;
    u_int64_t num_devices_to_pair = 0;
    u_int64_t pair_device_id = 0;
    Device* pair_dev = NULL;
    macaddr_t pair_macaddr;
    char dev1_key[DEVICEPAIR_KEY_SIZE];
    char dev2_key[DEVICEPAIR_KEY_SIZE];

    // Set up signal handler for SIGINT (Ctrl+C)
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    // global device
    extern Device* dev;

    // Initialize random seed
    srand(time(NULL));

    if (argc < 2)
    {
        al_log("Needs atleast 1 arg: device_id as int");
        exit(FAIL);
    }
    device_id = atoi((char*)(argv[1]));

    // Create device for this process.
    result = Device_create_malloc(device_id, &dev);
    if (result != SUCCESS)
    {
        al_log("Failed to create device.");
        exit(FAIL);
    }
    
    result = gen_macaddr_from_int(device_id, &macaddr);
    if (result != SUCCESS)
    {
        al_log("Failed to generate MAC address.");
        exit(FAIL);
    }

    Device_assignaddress(dev, macaddr);
    print_dev_all(dev);   

    // Additional args are integers for which device to connect to,
    if (argc > 2)
    {
        num_devices_to_pair = atoi((char*)(argv[2]));
        u_int64_t start_num = atoi((char*)(argv[3]));
        // num_devices_to_pair = argc - 2;
        al_log("Creating external devices and pairing records.");

        for (i = 0; i < num_devices_to_pair; i++)
        {
            // Get index + 2. First arg is device_id.
            // pair_device_id = atoi((char*)(argv[i+2]));
            pair_device_id = start_num+i;

            // Create device. Also, mac adddress and key creation exchange simulation.
            result = Device_create_malloc(pair_device_id, &pair_dev);
            if (result != SUCCESS)
            {
                al_log("Failed to create device.");
                exit(FAIL);
            }
            
            // Simulate exchanging the macaddr and key by using programattic/static generation functions.
            result = gen_macaddr_from_int(pair_device_id, &pair_macaddr);
            if (result != SUCCESS)
            {
                al_log("Failed to generate MAC address.");
                exit(FAIL);
            }
            Device_assignaddress(pair_dev, pair_macaddr);

            gen_key_from_ints(device_id, pair_device_id, dev1_key, sizeof(dev1_key));
            gen_key_from_ints(pair_device_id, device_id, dev2_key, sizeof(dev2_key));
            result = Device_createpairingrecord(dev, pair_dev, dev1_key, dev2_key, &devpair);
            // al_log("Created device pairing record for device.");

        }

        al_log("Device pairs:\n");
        Device_printallpairs(dev);

    }

    if (argc == 5)
    {
        al_log("This process will end after %d packets.",TOTAL_TO_END);
    }

    // Start threads
    pthread_create(&send_thread, NULL, sim_send_data_thread, NULL);
    pthread_create(&recv_thread, NULL, sim_recv_data_thread, NULL);

    // Join threads
    pthread_join(send_thread, NULL);
    pthread_join(recv_thread, NULL);
    usleep(10);

    // Profiling output
    // double total_monotonic = al_send_total + total_latency_sec;
    // al_log("STAT [Dev %lu-S2] - Total Send Full CPU time: %lf", dev->id, al_send_cpu_total);
    // al_log("STAT [Dev %lu-S5] - Total Send Total 'Latency' time (s): %lf", dev->id, total_latency_sec);
    // al_log("STAT [Dev %lu-S6] - Total Sent packets: %d", dev->id, total_send_packet);
    // al_log("STAT [Dev %lu-S7] - Inside wrap/encryption: %lf", dev->id, al_profile_wrap_total);
    // al_log("STAT [Dev %lu-S8] - Inside wrap/lookup: %lf", dev->id, al_profile_wrap_lookup_total);

     // Print stats.
    al_log("Printing stats:");
    char results_buffer[5000] = {0};
    char *results_ptr = results_buffer;

    double total_processing_sec = total_recv_packet * ((SIM_IO_TIME+sizeof(L2_MTU)) / 1e6);
    double total_latency_sec = total_send_packet * TRANSMISSION_LATENCY / 1e6;
    double total_latency_baseline_sec = total_send_packet * TRANSMISSION_LATENCY_BASELINE / 1e6;

    results_ptr += sprintf(results_ptr, "{");
    results_ptr += sprintf(results_ptr, "\"dev_id\": %lu, ", dev->id);
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


    // double total_monotonic = al_recv_total + total_processing_sec;
    // double total_monotonic = al_recv_total + al_recv_simwork_total;
    // al_log("STAT [Dev %lu-R1] - Total Recv Unwrap CPU time: %lf", dev->id, al_recv_cpu_unwrap_total);
    // al_log("STAT [Dev %lu-R2] - Total Recv Pseudo CPU time: %lf", dev->id, al_recv_pseudo_total);
    // al_log("STAT [Dev %lu-R3] - Total Recv Full CPU time: %lf", dev->id, al_recv_cpu_total);
    // al_log("STAT [Dev %lu-R4] - Total Recv Unwrap Monotonic time: %lf", dev->id, al_recv_unwrap_total);
    // al_log("STAT [Dev %lu-R5] - Total Recv Total Simulated work time (s): %lf", dev->id, al_recv_simwork_total);
    // al_log("STAT [Dev %lu-R6] - Total Recv Full Monotonic time: %lf", dev->id, al_recv_total);
    // al_log("STAT [Dev %lu-R7] - Total Recv packets: %d", dev->id, total_recv_packet);
    // al_log("STAT [Dev %lu-R8] - Total Recv NoPair packets: %d", dev->id, total_no_pair);
    // al_log("STAT [Dev %lu-R9] - Inside unwrap/Decryption: %lf", dev->id, al_profile_unwrap_total);
    // al_log("STAT [Dev %lu-R9] - Inside unwrap/Lookup: %lf", dev->id, al_recv_lookup_total);
    // al_log("STAT [Dev ] - Total Recv Total 'Processing' time (s): %lf", dev->id, total_processing_sec);
    // al_log("STAT [Dev %d-7] - Total Recv Total Monotonic Overall time (s): %lf", dev->id, total_monotonic);
    
    // Cleanup
    Device_delete(&dev);

    // End of Demo.
    al_log("End of single device simulation.");
    return result;

}
