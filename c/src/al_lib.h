/******************************************************************************
Abstraction Layer
Author: Christopher Ellis
Implementation in C
******************************************************************************/

#ifndef AL_LIB_H
#define AL_LIB_H

#define AL_SIM 1

#ifdef AL_SIM
#include "sha_tp.h"
#include "aes.h"
#else
#include "include/sha_tp.h"
#include "include/aes.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <linux/types.h>
#include <stdatomic.h>
#include <time.h>

// Define functions for ebpf program vs c library.
#ifdef AL_EBPF

    #define trace_printk(fmt, ...) do { \
	char _fmt[] = fmt; \
	bpf_trace_printk(_fmt, sizeof(_fmt), ##__VA_ARGS__); \
	} while (0)

    #define al_print trace_printk 
    #define al_memset __builtin_memset
    #define al_memcpy __builtin_memcpy
    #define al_memcmp __builtin_memcmp

#else

    #define al_print printf
    #define al_memset memset
    #define al_memcpy memcpy
    #define al_memcmp memcmp

#endif

// Proiling macros
#define ELAPSED_TIME(profile_var) ( \
    (profile_var).total += ((profile_var).end.tv_sec - (profile_var).start.tv_sec) + \
    ((profile_var).end.tv_nsec - (profile_var).start.tv_nsec) / 1e9 \
)

#define START_CLOCK(profile_var) ( \
    clock_gettime(CLOCK_MONOTONIC, &(profile_var).start) \
)

#define STOP_CLOCK(profile_var) \
    do { \
        clock_gettime(CLOCK_MONOTONIC, &(profile_var).end); \
        ELAPSED_TIME(profile_var); \
    } while (0)


#define START_CLOCK_CPU(profile_var) ( \
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &(profile_var).start) \
)

#define STOP_CLOCK_CPU(profile_var) \
    do { \
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &(profile_var).end); \
        ELAPSED_TIME(profile_var); \
    } while (0)


// Transmission time
#define BLE_1M_PHY 1000000   // 1 Mbps for BLE 4.x
#define BLE_2M_PHY 2000000   // 2 Mbps for BLE 5.x
#define BLE_TX_TIME_US(bytes, phy_speed) ((bytes * 8.0 * 1000000) / phy_speed)


// Print
extern int print_verbose;
extern int print_very_verbose;

#define al_log(fmt, ...) \
    do { \
        al_print("[AL] " fmt, ##__VA_ARGS__); \
        al_print("\n"); \
    } while(0)

#define al_log_v(fmt, ...) \
    do { \
        if (print_verbose) { \
            al_print("[AL] " fmt, ##__VA_ARGS__); \
            al_print("\n"); \
        } \
    } while(0)

#define al_log_vv(fmt, ...) \
    do { \
        if (print_very_verbose) { \
            al_print("[AL] " fmt, ##__VA_ARGS__); \
            al_print("\n"); \
        } \
    } while(0)

#define al_log_error(fmt, ...) \
    do { \
        al_print("[AL] ERROR: " fmt, ##__VA_ARGS__); \
        al_print("\n"); \
    } while(0)

#define print_space() al_print("\n")

#define VERBOSE_CALL(func, ...) \
    do { \
        if (print_verbose) { \
            func(__VA_ARGS__); \
        } \
    } while (0)


/******************************************************************************
 * Defines
******************************************************************************/

#define IP_ADDR_SIZE 16
#define ADDR_SIZE IP_ADDR_SIZE
#define MAC_ADDR_SIZE 6

// #define L2_MTU 1500
// #define L2_MTU 16
// #define L2_MTU 32
// #define L2_MTU 64
// #define L2_MTU 128
#define L2_MTU 256
// #define L2_MTU 512
// #define L2_MTU 1024
// #define L2_MTU 2048

// #define AL_MTU 1520
#define AL_MTU L2_MTU + L2_FRAME_PADDING + 20 
#define SMALL_MTU 64

#define AL_BLE_1M BLE_TX_TIME_US(sizeof(ALPacket), BLE_1M_PHY)
#define L2_BLE_1M BLE_TX_TIME_US(sizeof(L2Frame), BLE_1M_PHY)

#define TRANSMISSION_LATENCY AL_BLE_1M
#define TRANSMISSION_LATENCY_BASELINE L2_BLE_1M

// Use 18 as the total for the other datamembers of the L2 Frame
#define L2_FRAME_PADDING CBC_BLOCK_SIZE - ((L2_MTU + 18) % CBC_BLOCK_SIZE)

// Work simulation
#define SIM_IO_TIME 0.1 // microseconds

// Return codes
#define SUCCESS 0
#define FAIL 1
#define NO_PAIR 20

// Configurables
#define METHOD_CACHE 1
#define NUM_MAX_PAIRED_DEVICES 1024
#define NUM_TRANSMISSION_KEYS 10
#define TRANSMISSION_KEY_SIZE 6
#define DEVICEPAIR_KEY_SIZE 32
#define SOURCE_KEY_SIZE 32
#define NUM_LAST_SENT_ARRAY_SIZE 10
#define PAIRED_ENCRYPTION_KEY_SIZE SOURCE_KEY_SIZE
#define ENCRYPTION_KEY_SIZE SOURCE_KEY_SIZE
#define NUM_PSEUDO_TKEYS 10
#define TK_NONCE_SIZE 8
#define SK_STR "ALsrc"
#define ENC_STR "ALenc"

#define AL_DOMAIN_BIND "tcp://*"
#define AL_DOMAIN_CONNECT "tcp://localhost"
#define AL_DOMAIN_UPPERLAYER "tcp://localhost"
#define AL_DOMAIN_UPPERLAYER_PORT_START 30000
#define AL_PROXY_PORT_IN 39000
#define AL_PROXY_PORT_OUT 39001

#define TRUE 1
#define FALSE 0
#define NO_DATA -1

// Encryption flags
#define CBC 1
// #define CTR 1
// #define ECB 1
#define CBC_BLOCK_SIZE 16



/******************************************************************************
 * Typedefs & Structs
******************************************************************************/

typedef struct device_struct Device;
typedef struct devicepair_struct DevicePair;
typedef struct pseudopair_struct PseudoPair;
typedef struct transmissionkey_struct TransmissionKey;
typedef struct encryptionkey_struct EncryptionKey;
typedef struct al_packet_struct ALPacket;
typedef struct l2_frame_struct L2Frame;
typedef struct al_profile_struct ALProfile;
typedef uint8_t result_t;
typedef uint8_t tkey_t[TRANSMISSION_KEY_SIZE];
typedef uint8_t ekey_t[ENCRYPTION_KEY_SIZE];
typedef uint8_t macaddr_t[MAC_ADDR_SIZE];

struct pseudopair_struct {
    tkey_t tkey[NUM_PSEUDO_TKEYS];
};


struct device_struct {
    u_int64_t id;
    macaddr_t macaddr;
    DevicePair* device_pairs;
    PseudoPair pseudopair;
    tkey_t last_sent[NUM_LAST_SENT_ARRAY_SIZE];
    uint32_t last_sent_index;

};


struct transmissionkey_struct {
    ekey_t ekey;
    tkey_t tkey;
    struct AES_ctx aes_ctx;
};


struct devicepair_struct {
    Device* device;
    DevicePair* next;
    // int address;
    char key[DEVICEPAIR_KEY_SIZE];
    char other_key[32];
    char paired_key[32];
    char source_key[32];
    char source_other_key[32];
    char paired_encryption_key[32];
    uint32_t current_tkey_src;
    uint32_t current_tkey_recv;
    uint32_t nonce_counter;
    TransmissionKey transmission_keys_src[NUM_TRANSMISSION_KEYS];
    TransmissionKey transmission_keys_recv[NUM_TRANSMISSION_KEYS];
    uint8_t hash_rnd_bytes[3];
    uint8_t hash_tk_digest[3];
    uint8_t hash_tkey[6];
};



// Non payload/padding bytes == 18, so use this as padding number modulo.
struct l2_frame_struct {
    macaddr_t dst;
    macaddr_t src;
    uint16_t ethertype;
    uint8_t payload[L2_MTU];
    uint8_t padding[L2_FRAME_PADDING];
    uint32_t crc;
};


// May need to pack this for compiler
struct al_packet_struct {
    tkey_t tkey;
    uint8_t data[sizeof(L2Frame)];
};


typedef struct DevicePairList
{
    

} DevicePairlist_t;


struct al_profile_struct
{
    struct timespec start;
    struct timespec end;
    double total;
};


/******************************************************************************
 * Function Signatures
******************************************************************************/

// Utils
uint8_t* gen_rnd_bytes(size_t num_bytes);
result_t gen_rnd_bytes_inplace(uint8_t *buffer, size_t num_bytes);
result_t gen_macaddr_from_int(u_int32_t id, macaddr_t* macaddr);
result_t gen_key_from_ints(int src_device_id, int dst_device_id, char* key_buffer, size_t buffer_size);
void printmac(macaddr_t macaddr);
void print_l2frame_all(L2Frame* frame, int printsize);
void print_alpacket_all(ALPacket* packet, int printsize);
void print_dev_all(Device* dev);
double simulate_floating_point_work(uint8_t* data, int size);


// Device
result_t Device_create_malloc(uint32_t id, Device** device);
result_t Device_init(uint32_t id, Device* new_device);
result_t Device_delete(Device** device);
result_t Device_assignaddress(Device* dev, macaddr_t macaddr);
result_t Device_pairdevices(Device* dev1, Device* dev2);
DevicePair* Device_checkpair_by_address(Device* dev, macaddr_t macaddr);
DevicePair* Device_checkpair_by_tkey(Device* dev, tkey_t tkey);
result_t Device_wrappacket(Device* dev, uint8_t* data, ALPacket* packet);
result_t Device_unwrappacket(Device* dev, ALPacket* packet, uint8_t* data);
result_t Device_preparepseudo(Device* dev, L2Frame* l2frame, ALPacket* pseudopacket);
result_t Device_check_lastsent(Device* dev, tkey_t tkey);
result_t Device_set_lastsent(Device* dev, tkey_t tkey);
void Device_printallpairs(Device* dev);

// DevicePair
result_t DevicePair_create_malloc(Device* device, DevicePair** devpair, char* this_key);
result_t DevicePair_init(Device* device, DevicePair* new_devicepair, char* this_key);
result_t Device_createpairingrecord(Device* dev, Device* external_dev, char* this_key, char* inc_key, DevicePair** devpair);
result_t DevicePair_create_transmission_keys(DevicePair* devpair, uint8_t num);
result_t DevicePair_create_transmission_key_at_offset(DevicePair* src, uint32_t offset);
result_t DevicePair_do_key_exchange(Device one, Device two);
result_t DevicePair_encrypt_data(DevicePair* paired_device, uint32_t key_index, uint8_t* data, size_t size);
result_t DevicePair_decrypt_data(DevicePair* devpair, uint32_t key_index, uint8_t* data, size_t size);
result_t DevicePair_set_new_enc_ctx(DevicePair* src, DevicePair* dst);


#endif
