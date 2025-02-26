/******************************************************************************
Abstraction Layer
Author: Christopher Ellis
Implementation in C
******************************************************************************/

#include "al_lib.h"

#ifndef AL_SIM
#include <android/log.h>
#define al_print LOGI // android
#define  LOG_TAG    "AL"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#endif

/******************************************************************************
Profiling
******************************************************************************/

ALProfile profile_send_lookup = {0};
ALProfile profile_encrypt = {0};
ALProfile profile_recv_lookup = {0};
ALProfile profile_decrypt = {0};
ALProfile profile_simwork = {0};
ALProfile profile_pseudo = {0};
ALProfile profile_send_lookup_cpu = {0};
ALProfile profile_encrypt_cpu = {0};
ALProfile profile_recv_lookup_cpu = {0};
ALProfile profile_decrypt_cpu = {0};
ALProfile profile_simwork_cpu = {0};
ALProfile profile_pseudo_cpu = {0};


/******************************************************************************
 * Globals
******************************************************************************/

uint8_t g_aes_default_iv[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };

int print_verbose = TRUE;
// int print_verbose = FALSE;
int print_very_verbose = FALSE;


/******************************************************************************
 * Utils
******************************************************************************/

result_t gen_rnd_bytes_inplace(uint8_t *buffer, size_t num_bytes)
{
    size_t i;

    for (i = 0; i < num_bytes; i++)
    {
        buffer[i] = rand();
    }

    return SUCCESS;
};


void print_mac(macaddr_t macaddr)
{
    al_print("MAC Addr: %02X:%02X:%02X:%02X:%02X:%02X\n", macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
};


void print_dev_all(Device* dev)
{
    unsigned char macstr[MAC_ADDR_SIZE*2 + 5 + 1] = {0};
    snprintf(macstr, sizeof(macstr), "%02X:%02X:%02X:%02X:%02X:%02X", dev->macaddr[0], dev->macaddr[1], dev->macaddr[2], dev->macaddr[3], dev->macaddr[4], dev->macaddr[5]);
    al_print("<Device ID: %lu, MAC Addr: %s>\n", dev->id, macstr);
};


void print_devpair_all(DevicePair* devpair)
{

    int i, j;
    char *p;
    unsigned char tkey_str[TRANSMISSION_KEY_SIZE*2 + TRANSMISSION_KEY_SIZE-1 + 1] = {0};
    unsigned char ekey_str[ENCRYPTION_KEY_SIZE*2 + 1] = {0};
    unsigned char macstr[MAC_ADDR_SIZE*2 + 5 + 1] = {0};
    
    snprintf(macstr, sizeof(macstr), "%02X:%02X:%02X:%02X:%02X:%02X",
        devpair->device->macaddr[0], devpair->device->macaddr[1], devpair->device->macaddr[2],
        devpair->device->macaddr[3], devpair->device->macaddr[4], devpair->device->macaddr[5]);
    
    al_print("<DevicePair Device(ID: %lu, MAC Addr: %s), Key: %s, Other Key: %s, Paired Key: %s, Source Key: %s, PEK: %s, CurTkeySrc: %d, CurTkeyRecv: %d\n",
        devpair->device->id,
        macstr,
        devpair->key,
        devpair->other_key,
        devpair->paired_key,
        devpair->source_key,
        devpair->paired_encryption_key,
        devpair->current_tkey_src,
        devpair->current_tkey_recv);
    printf("\n");

    al_print("\tTransmission / Encryption Keys (Src):\n");
    for (i = 0; i < NUM_TRANSMISSION_KEYS; i++)
    {
        snprintf(tkey_str, sizeof(tkey_str), "%02X:%02X:%02X:%02X:%02X:%02X",
            devpair->transmission_keys_src[i].tkey[0], devpair->transmission_keys_src[i].tkey[1],
            devpair->transmission_keys_src[i].tkey[2], devpair->transmission_keys_src[i].tkey[3],
            devpair->transmission_keys_src[i].tkey[4], devpair->transmission_keys_src[i].tkey[5]);

        p = ekey_str;
        for (j = 0; j < sizeof(ekey_t); j++) {
            p += sprintf(p, "%02x", devpair->transmission_keys_src[i].ekey[j]);
        }
            
        al_print("\t%d: %s / %s\n", i, tkey_str, ekey_str);
    }
    printf("\n");

    al_print("\tTransmission / Encryption Keys (Recv):\n");
    for (i = 0; i < NUM_TRANSMISSION_KEYS; i++)
    {
        snprintf(tkey_str, sizeof(tkey_str), "%02X:%02X:%02X:%02X:%02X:%02X",
            devpair->transmission_keys_recv[i].tkey[0], devpair->transmission_keys_recv[i].tkey[1],
            devpair->transmission_keys_recv[i].tkey[2], devpair->transmission_keys_recv[i].tkey[3],
            devpair->transmission_keys_recv[i].tkey[4], devpair->transmission_keys_recv[i].tkey[5]);

        p = ekey_str;
        for (j = 0; j < sizeof(ekey_t); j++) {
            p += sprintf(p, "%02x", devpair->transmission_keys_recv[i].ekey[j]);
        }
            
        al_print("\t%d: %s / %s\n", i, tkey_str, ekey_str);
    }
    printf("\n");


};


void print_alpacket_all(ALPacket* packet, int printsize)
{

    unsigned char tkey_str[TRANSMISSION_KEY_SIZE*2 + TRANSMISSION_KEY_SIZE-1 + 1] = {0};
    unsigned char data_str[sizeof(packet->data)*2 + 1] = {0};
    snprintf(tkey_str, sizeof(tkey_str), "%02X:%02X:%02X:%02X:%02X:%02X", packet->tkey[0], packet->tkey[1], packet->tkey[2], packet->tkey[3], packet->tkey[4], packet->tkey[5]);
    char *p = data_str;

    if(printsize > sizeof(packet->data) || printsize == 0)
    {
        printsize = sizeof(packet->data);
    }

    for (size_t i = 0; i < printsize; i++) {
        p += sprintf(p, "%02x", packet->data[i]);
    }
    al_print("<ALPacket tkey: %s, data: %s>\n", tkey_str, data_str);

};


void print_l2frame_all(L2Frame* frame, int printsize)
{

    unsigned char dst_str[MAC_ADDR_SIZE*2 + MAC_ADDR_SIZE-1 + 1] = {0};
    unsigned char src_str[MAC_ADDR_SIZE*2 + MAC_ADDR_SIZE-1 + 1] = {0};
    unsigned char payload_str[L2_MTU*2 + 1] = {0};
    char *p = payload_str;

    if(printsize > sizeof(frame->payload) || printsize == 0)
    {
        printsize = sizeof(frame->payload);
    }
    

    snprintf(dst_str, sizeof(dst_str), "%02X:%02X:%02X:%02X:%02X:%02X", frame->dst[0], frame->dst[1], frame->dst[2], frame->dst[3], frame->dst[4], frame->dst[5]);
    snprintf(src_str, sizeof(src_str), "%02X:%02X:%02X:%02X:%02X:%02X", frame->src[0], frame->src[1], frame->src[2], frame->src[3], frame->src[4], frame->src[5]);
    for (size_t i = 0; i < printsize; i++) {
        p += sprintf(p, "%02x", frame->payload[i]);
    }
    al_print("<L2Frame dst: %s, src: %s, payload: %s>\n", dst_str, src_str, payload_str);

};

result_t gen_macaddr_from_int(u_int32_t id, macaddr_t* macaddr)
{
    result_t result = SUCCESS;
    int i = 0;
    
    uint8_t mac_byte = (uint8_t)id;

    memset(macaddr, 0, sizeof(macaddr_t));

    for (size_t i = 0; i < sizeof(macaddr_t); i++)
    {
        *(*(macaddr) + i) = (id >> (i * 8)) & 0xFF;  // Extract each byte
    }

    return result;
}

result_t gen_key_from_ints(int src_device_id, int dst_device_id, char* key_buffer, size_t buffer_size)
{
    result_t result = SUCCESS;
    int i = 0;
    
    memset(key_buffer, 0, buffer_size);
    snprintf(key_buffer, buffer_size, "%d_AL_%d", src_device_id, dst_device_id);

    return result;
}

double simulate_floating_point_work(uint8_t* data, int size) {
    double x = 0.1;
    for (int i = 0; i < size; i++) {
        x = x * 1.001 - 0.001 + rand() + data[i];  // Some arbitrary floating-point operation.
        data[i] = (uint8_t)x;
    }
    return x;
}

/******************************************************************************
 * Device
******************************************************************************/

result_t Device_create_malloc(uint32_t id, Device** device)
{

    result_t result = SUCCESS;

    // Allocate
    Device* new_device = (Device*)malloc(sizeof(Device));
    if (new_device == NULL)
    {
        al_log_error("Device init failed on Device malloc."); 
        return FAIL;
    }

    // Init
    result = Device_init(id, new_device);
    if (result != SUCCESS)
    {
        al_log_error("Device init failed on Device init."); 
        return result;
    }

    // Assign
    *device = new_device;

    return result;
}

result_t Device_init(uint32_t id, Device* new_device)
{

    result_t result = SUCCESS;

    // Initialize vars
    new_device->id = id;
    new_device->device_pairs = NULL;
    new_device->last_sent_index = 0;
    al_memset(new_device->macaddr, 0, sizeof(new_device->macaddr));
    al_memset(new_device->last_sent, 0, sizeof(new_device->last_sent));

    // Create psuedo tkeys.
    int i;
    for (i = 0; i < NUM_PSEUDO_TKEYS; i++)
    {
        result = gen_rnd_bytes_inplace(new_device->pseudopair.tkey[i], sizeof(tkey_t));
        if (result != SUCCESS)
        {
            al_log_error("Device init failed on pseudopair generation."); 
            return result;
        }
    }

    return result;

};

result_t Device_assignaddress(Device* dev, macaddr_t macaddr)
{
    memcpy(dev->macaddr, macaddr, sizeof(macaddr_t));
    return SUCCESS;
}

// This will need to fully iterate through all 
result_t Device_delete(Device** dev)
{
    // DevicePair* devpair = *(dev->device_pairs);
    // DevicePair* nextpair = NULL;

    // Free device pairs
    // while (devpair != NULL)
    // {
        // nextpair = devpair->next;
        // free(devpair);
        // devpair = nextpair;

    // }

    // Free device;
    free(*dev);
    *dev = NULL;
    return SUCCESS;
};


result_t Device_check_lastsent(Device* dev, tkey_t tkey)
{
    int i = 0;
    size_t size = -1;
    result_t result = FALSE;
    for (i = 0; i < NUM_LAST_SENT_ARRAY_SIZE; i++)
    {
        size = memcmp(dev->last_sent[i], tkey, sizeof(tkey_t));
        if(size == 0)
        {
            return TRUE;
        } 
    }

    return FALSE;
};

result_t Device_set_lastsent(Device* dev, tkey_t tkey)
{
    memcpy(dev->last_sent[dev->last_sent_index], tkey, sizeof(tkey_t));
    dev->last_sent_index++;
    if (dev->last_sent_index == NUM_LAST_SENT_ARRAY_SIZE)
    {
        dev->last_sent_index = 0;
    }

    return SUCCESS;
}


result_t Device_createpairingrecord(Device* dev, Device* external_dev, char* this_key, char* inc_key, DevicePair** devpair)
{
    char paired_key[DEVICEPAIR_KEY_SIZE];
    char paired_encryption_key[PAIRED_ENCRYPTION_KEY_SIZE];
    DevicePair* new_devpair = NULL;
    int i = 0;
    uint8_t result = SUCCESS;

    result = DevicePair_create_malloc(external_dev, &new_devpair, this_key);
    if (result != SUCCESS)
    {
        al_log_error("Failed to create device pair record.");
        return FAIL;
    }

    // Save key incoming device key.
    memcpy(new_devpair->other_key, inc_key, sizeof(new_devpair->other_key));
    
    // Create paired key and save.
    // new_devpair->paired_key = new_devpair->key ^ new_devpair->other_key;
    for (i = 0; i < DEVICEPAIR_KEY_SIZE; i++)
    {
        // paired_key[i] = new_devpair->key[i] ^ inc_key[i];
        paired_key[i] = '\x41';
    }
    memcpy(new_devpair->paired_key, paired_key, sizeof(new_devpair->paired_key));

    // Create source keys.
    hkdf(SHA256, paired_key, sizeof(paired_key)/2, new_devpair->key, sizeof(new_devpair->key), SK_STR, sizeof(SK_STR), new_devpair->source_key, sizeof(new_devpair->source_key));
    hkdf(SHA256, paired_key, sizeof(paired_key)/2, new_devpair->other_key, sizeof(new_devpair->other_key), SK_STR, sizeof(SK_STR), new_devpair->source_other_key, sizeof(new_devpair->source_other_key));
    // new_devpair->source_key = hkdf(paired_key, devpair1->key, SK_STR, SOURCE_KEY_SIZE);
    // devpair2->source_key = hkdf(paired_key, devpair2->key, "ALsrc", SOURCE_KEY_SIZE);
    
    // Create encryption keys
    hkdf(SHA256, NULL, 0, paired_key, sizeof(paired_key)/2, ENC_STR, sizeof(ENC_STR), paired_encryption_key, sizeof(paired_encryption_key));
    memcpy(new_devpair->paired_encryption_key, paired_encryption_key, sizeof(new_devpair->paired_encryption_key));

    // Create Tranmission Keys
    result = DevicePair_create_transmission_keys(new_devpair, NUM_TRANSMISSION_KEYS);
    if (result != SUCCESS)
    {
        al_log_error("Failed to create transmission keys for devpair.");
        return FAIL;
    }

    // Add to end of list for the device.
    DevicePair** lastpair = &(dev->device_pairs);
    if (*lastpair == NULL)
    {   
        *lastpair = new_devpair;
    }
    else
    {
        while ((*lastpair)->next != NULL)
        {
            lastpair = &((*lastpair)->next);
        }
        (*lastpair)->next = new_devpair;

    }

    // Assign
    *devpair = new_devpair;

}


void Device_printallpairs(Device* dev)
{
    DevicePair* devpair = dev->device_pairs;
    
    if (devpair == NULL)
    {
        al_print("\t- No pairs.\n\n");
    }

    while (devpair != NULL)
    {
        print_devpair_all(devpair);
        devpair = devpair->next;
    }
}


result_t Device_preparepseudo(Device* dev, L2Frame* l2frame, ALPacket* pseudopacket)
{
    result_t result = SUCCESS;

    START_CLOCK_CPU(profile_pseudo_cpu);
    START_CLOCK(profile_pseudo);
    gen_rnd_bytes_inplace(pseudopacket->data, sizeof(pseudopacket->data));
    al_memcpy(pseudopacket->tkey, dev->pseudopair.tkey[rand() % NUM_TRANSMISSION_KEYS], sizeof(tkey_t));
    STOP_CLOCK_CPU(profile_pseudo_cpu);
    STOP_CLOCK(profile_pseudo);

    return result;

};



/******************************************************************************
 * DevicePair
******************************************************************************/

result_t DevicePair_create_malloc(Device* device, DevicePair** devpair, char* this_key)
{

    result_t result = SUCCESS;

    // Allocate
    DevicePair* new_devpair = (DevicePair*)malloc(sizeof(DevicePair));
    if (new_devpair == NULL)
    {
        al_log_error("DevicePair init failed on DevicePair malloc."); 
        return FAIL;
    }

    // Init
    result = DevicePair_init(device, new_devpair, this_key);
    if (result != SUCCESS)
    {
        al_log_error("DevicePair init failed on init."); 
        return result;
    }

    *devpair = new_devpair;

    return result;

}


result_t DevicePair_init(Device* device, DevicePair* new_devicepair, char* this_key)
{

    result_t result = SUCCESS;

    // Set vars
    new_devicepair->device = device; // Assign the device so we can reference for addresses and free later.
    new_devicepair->next = NULL;
    new_devicepair->current_tkey_src = 0;
    new_devicepair->current_tkey_recv = 0;
    new_devicepair->nonce_counter = 0;
    memset(new_devicepair->other_key, 0, sizeof(new_devicepair->other_key));
    memset(new_devicepair->paired_key, 0, sizeof(new_devicepair->paired_key));
    memset(new_devicepair->source_key, 0, sizeof(new_devicepair->source_key));
    memset(new_devicepair->source_other_key, 0, sizeof(new_devicepair->source_other_key));
    memset(new_devicepair->paired_encryption_key, 0, sizeof(new_devicepair->paired_encryption_key));
    memset(new_devicepair->hash_rnd_bytes, 0, sizeof(new_devicepair->hash_rnd_bytes));
    memset(new_devicepair->hash_tk_digest, 0, sizeof(new_devicepair->hash_tk_digest));

    // Generate key
    if (this_key == 0)
    {
        result = gen_rnd_bytes_inplace(new_devicepair->key, sizeof(new_devicepair->key));
        if (result != SUCCESS)
        {
            al_log_error("Failed to generate random bytes for new device pair key,");
        }
    }
    else
    {
        memcpy(new_devicepair->key, this_key, sizeof(new_devicepair->key));
    }

    return result;

};


result_t DevicePair_create_transmission_keys(DevicePair* devpair, uint8_t num)
{
    uint32_t i;
    result_t result = SUCCESS;

    if (num > NUM_TRANSMISSION_KEYS)
    {
        al_log_error("Can't create more Transmissions keys than allowed.");
        return FAIL;
    }

    for(i = 0; i < num; i++)
    {
        result = DevicePair_create_transmission_key_at_offset(devpair, i);
        if (result != SUCCESS)
        {
            al_log_error("Failed to create tranmissions key.");
            return FAIL;
        }
    } 

    return result;

};


result_t DevicePair_create_transmission_key_at_offset(DevicePair* devpair, uint32_t offset)
{   
    uint32_t tkey;
    uint32_t ekey;
    result_t result = SUCCESS;

    int keygen;

    char nonce[TK_NONCE_SIZE] = {0};
    snprintf(nonce, sizeof(nonce), "%d", devpair->nonce_counter);
    devpair->nonce_counter++;

    // Source
    keygen = hkdf(SHA256, (char*)nonce, sizeof(nonce), (char*)devpair->source_key, sizeof(devpair->source_key)/2, SK_STR, sizeof(SK_STR), devpair->transmission_keys_src[offset].tkey, sizeof(tkey_t));
    keygen = hkdf(SHA256, (char*)nonce, sizeof(nonce), (char*)devpair->source_key, sizeof(devpair->source_key)/2, ENC_STR, sizeof(ENC_STR), devpair->transmission_keys_src[offset].ekey, sizeof(ekey_t));

    // Recv
    keygen = hkdf(SHA256, (char*)nonce, sizeof(nonce), (char*)devpair->source_other_key, sizeof(devpair->source_other_key)/2, SK_STR, sizeof(SK_STR), devpair->transmission_keys_recv[offset].tkey, sizeof(tkey_t));
    keygen = hkdf(SHA256, (char*)nonce, sizeof(nonce), (char*)devpair->source_other_key, sizeof(devpair->source_other_key)/2, ENC_STR, sizeof(ENC_STR), devpair->transmission_keys_recv[offset].ekey, sizeof(ekey_t));

    // Set encryption contexts.
    // Note: set different default IV, but using the same is fine for simulation demo.
    uint8_t g_aes_default_iv[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
    AES_init_ctx_iv(&(devpair->transmission_keys_src[offset].aes_ctx), devpair->transmission_keys_src[offset].ekey, g_aes_default_iv);
    AES_init_ctx_iv(&(devpair->transmission_keys_recv[offset].aes_ctx), devpair->transmission_keys_recv[offset].ekey, g_aes_default_iv);
    
    return result;

};


// This needs to get a device pair based on the MAC address from another layer,
// this is likely exclusively for Sending
DevicePair* Device_getpair_by_address(Device* dev, macaddr_t macaddr)
{

    DevicePair* devpair = (dev->device_pairs);
    int i = 0;
    // TESTING FOR LOOKUP ONLY. 
    DevicePair* returndevpair = NULL;

    // al_log("Search for pair...");

    // Try a pairing record while we have them.
    while (devpair != NULL)
    {

        // Check for a matching address.
        if (memcmp(devpair->device->macaddr, macaddr, sizeof(macaddr_t)) == 0)
        {
            // We found it! Set the index found.
            // al_log_v("Found pair!");
            returndevpair = devpair; // TESTING FOR LOOKUPONLY. This will cause worst case lookup.
            #ifdef METHOD_CACHE
            return devpair;
            #endif
        }

        // Try another pairing record.
        devpair = devpair->next;
    }
    

    // This will return a valid pair record, or NULL if not found.
    if (returndevpair != NULL) // TESTING FOR LOOKUP ONLY
    {
        devpair = returndevpair;
    }

    if (devpair == NULL)
    {
        al_log("No Device pair found when searching by address!");
    }

    return devpair;

};


// This needs to get a device pair based on a trasnmission key,
DevicePair* Device_getpair_by_tkey(Device* dev, tkey_t tkey, uint32_t* paired_index)
{

    DevicePair* devpair = (dev->device_pairs);
    int i = 0;
    // TESTING FOR LOOKUP ONLY. 
    DevicePair* returndevpair = NULL;

    // al_log_vv("Searching for pair...");


    // Try a pairing record while we have them.
    while (devpair != NULL)
    {

    #ifdef METHOD_CACHE

        // Check for a matching key.
        for (i = 0; i < NUM_TRANSMISSION_KEYS; i++)
        {   
            if (al_memcmp(devpair->transmission_keys_recv[i].tkey, tkey, sizeof(tkey_t)) == 0)
            {
                // We found it! Set the index found.
                // al_log_vv("Found pair!\n");
                *paired_index = i;
                // return devpair;
                returndevpair = devpair; // TESTING FOR LOOKUPONLY. This will cause worst case lookup.
            }
        }

    #else // Hash Method
        char* tkey_ptr = (char*)tkey;
        char rnd_bytes[3] = {0}; // Get random bytes from the first 3 bytes of tkey.
        char tk_digest[3] = {0}; // Get digest from the LAST 3 bytes of tkey
        char hash_combined[sizeof(rnd_bytes) + sizeof(devpair->paired_encryption_key)] = {0};
        // Note: Using USHAMaxHashSize instead of 32 because of the compiler warning. This appears to be a bug in
        // the library ebcause the output for SHA_256, which we use, is 32 and not 64 as the compiler warning hints.
        // This is the same in the other use of hash method.
        uint8_t hmac_output[USHAMaxHashSize] = {0};

        // Build buffers
        // Dissect tkey
        memset(rnd_bytes, 0, sizeof(rnd_bytes));
        memset(tk_digest, 0, sizeof(tk_digest));
        memcpy(rnd_bytes, tkey_ptr, sizeof(rnd_bytes));
        memcpy(tk_digest, tkey_ptr+3, sizeof(tk_digest));

        // Create combined rnd+enc_key
        memset(hash_combined, 0, sizeof(hash_combined));
        memcpy(hash_combined, rnd_bytes, sizeof(rnd_bytes));
        memcpy(hash_combined+sizeof(rnd_bytes), devpair->paired_encryption_key, sizeof(devpair->paired_encryption_key));

        // Create the digest.
        // int hmac(SHAversion whichSha, const unsigned char *message_array, int length, const unsigned char *key, int key_len, uint8_t digest[USHAMaxHashSize])
        memset(hmac_output, 0, sizeof(hmac_output));
        hmac(SHA256, hash_combined, sizeof(hash_combined), devpair->paired_encryption_key, sizeof(devpair->paired_encryption_key), hmac_output);

        // Compare, if match, the first 3 bytes of the digest == the tk_digest 
        if (al_memcmp(tk_digest, hmac_output, sizeof(tk_digest)) == 0)
        {
            // Found!
            // For now, simply use the first index and use the first index to encrypt as well.
            *paired_index = 0;
            returndevpair = devpair;
            #ifdef METHOD_CACHE
            return devpair; // NOTE: comment out for testing the worst case lookup time experiment. 
            #endif
        }

    #endif

        // Try another pairing record.
        devpair = devpair->next;
    }

    // This will return a valid pair record, or NULL if not found.
    if (returndevpair != NULL) // TESTING FOR LOOKUP ONLY
    {
        devpair = returndevpair;
    }

    if (devpair == NULL)
    {
        al_log("No pair found!\n");
    }

    return devpair;

};


result_t Device_wrappacket(Device* dev, uint8_t* data, ALPacket* packet)
{

    L2Frame* frame = NULL;
    DevicePair* devpair = NULL;
    int32_t tkey_index = NUM_TRANSMISSION_KEYS + 1;
    result_t result = SUCCESS; 

    // Make an l2 frame
    frame = (L2Frame*)data;

    // Lookup pair based on MAC destination
    START_CLOCK_CPU(profile_send_lookup_cpu);
    START_CLOCK(profile_send_lookup);
    devpair = Device_getpair_by_address(dev, frame->dst);
    STOP_CLOCK_CPU(profile_send_lookup_cpu);
    STOP_CLOCK(profile_send_lookup);
    if (devpair == NULL)
    {
        // Here is where we would drop packet, etc.
        al_log_v("Receieved frame but no pairing record for the address.");
        return FAIL;
    }


    // If pair, decide which key to use.
    #ifdef METHOD_CACHE
    tkey_index = rand() % NUM_TRANSMISSION_KEYS;
    #else
    tkey_index = 0;
    #endif
    if (tkey_index > NUM_TRANSMISSION_KEYS)
    {
        al_log_error("Tkey index exceeded max number of transission keys.");
        return FAIL;
    }
    devpair->paired_encryption_key;
    // Encrypt data
    START_CLOCK_CPU(profile_encrypt_cpu);
    START_CLOCK(profile_encrypt);
    DevicePair_encrypt_data(devpair, tkey_index, data, sizeof(L2Frame));
    STOP_CLOCK_CPU(profile_encrypt_cpu);
    STOP_CLOCK(profile_encrypt);

    // Create AL packet. Two methods, Hash or Cache.
    #ifdef METHOD_CACHE
        // Simply use the precalculated devpair key.
        al_memcpy(packet->tkey, devpair->transmission_keys_src[tkey_index].tkey, sizeof(tkey_t));
    #else
        // Calculate the hash... Random 3 bytes, 3 HMAC bytes
        char rnd_bytes[3] = {0}; // Random, used to compute the hmac. Store.
        char tk_digest[3] = {0}; // Resulting digest from the hash. Need to store in devpair to check later.
        char hash_combined[sizeof(rnd_bytes) + sizeof(devpair->paired_encryption_key)] = {0};
        char enc_key[32] = {0};

        // Generate randombytes..
        gen_rnd_bytes_inplace(rnd_bytes, sizeof(rnd_bytes));
        memcpy(hash_combined, rnd_bytes, sizeof(rnd_bytes));
        memcpy(enc_key, devpair->paired_encryption_key, sizeof(devpair->paired_encryption_key));
        memcpy(hash_combined+sizeof(rnd_bytes), devpair->paired_encryption_key, sizeof(devpair->paired_encryption_key));

        uint8_t hmac_output[USHAMaxHashSize] = {0};

        // Create the digest.
        // int hmac(SHAversion whichSha, const unsigned char *message_array, int length, const unsigned char *key, int key_len, uint8_t digest[USHAMaxHashSize])
        hmac(SHA256, hash_combined, sizeof(hash_combined), enc_key, sizeof(enc_key), hmac_output);

        // Save to devpair, random bytes and the digest.
        al_memcpy(devpair->hash_rnd_bytes, rnd_bytes, sizeof(devpair->hash_rnd_bytes));
        al_memcpy(devpair->hash_tk_digest, hmac_output, sizeof(devpair->hash_tk_digest));

        // Generate/Save the hash transmission key.
        // we would normally do a size check here, that size of rnd bytes and tk gigest is == to hash_tk_digest.
        al_memcpy(devpair->hash_tkey, devpair->hash_rnd_bytes, sizeof(devpair->hash_rnd_bytes));
        al_memcpy(devpair->hash_tkey + sizeof(devpair->hash_rnd_bytes), devpair->hash_tk_digest, sizeof(devpair->hash_tk_digest));

        // Copy gernated tkey to packet.
        al_memcpy(packet->tkey, devpair->hash_tkey, sizeof(tkey_t));

        // DevicePair_getpair_hash_tkey()
    #endif  

    // Copy the payload for any method.
    al_memcpy(packet->data, frame, sizeof(L2Frame));

    return result;

};

result_t Device_unwrappacket(Device* dev, ALPacket* packet, uint8_t* data)
{

    DevicePair* devpair = NULL;
    uint32_t tkey_index = NUM_TRANSMISSION_KEYS+1;
    result_t result = SUCCESS; 

    START_CLOCK_CPU(profile_recv_lookup_cpu);
    START_CLOCK(profile_recv_lookup);
    devpair = Device_getpair_by_tkey(dev, packet->tkey, &tkey_index);
    STOP_CLOCK_CPU(profile_recv_lookup_cpu);
    STOP_CLOCK(profile_recv_lookup);
    if (devpair == NULL)
    {
        al_log_v("Receieved packet but no pairing record");
        return NO_PAIR;
    }

    // Set current tindex
    devpair->current_tkey_recv = tkey_index;

    // Decrypt data of a successfully paired device.
    START_CLOCK_CPU(profile_decrypt_cpu);
    START_CLOCK(profile_decrypt);
    result = DevicePair_decrypt_data(devpair, tkey_index, packet->data, sizeof(L2Frame));
    STOP_CLOCK_CPU(profile_decrypt_cpu);
    STOP_CLOCK(profile_decrypt);

    // Reconstruct frame
    al_memcpy(data, packet->data, sizeof(L2Frame));

    return result;

};


result_t DevicePair_encrypt_data(DevicePair* devpair, uint32_t key_index, uint8_t* data, size_t size)
{

    struct AES_ctx* enc_ctx = NULL;

    enc_ctx = &(devpair->transmission_keys_src[key_index].aes_ctx);

    if (enc_ctx == NULL)
    {   
        al_log_error("Failed to get encryption context.");
        return FAIL;
    }

    // Padding
    if (size % CBC_BLOCK_SIZE != 0)
    {
        al_log_error("Data size isn't multiple of 16!");
    }

    // Encrypt
    AES_CBC_encrypt_buffer(enc_ctx, data, size);

    return SUCCESS; 

};


result_t DevicePair_decrypt_data(DevicePair* devpair, uint32_t key_index, uint8_t* data, size_t size)
{

    struct AES_ctx* enc_ctx = NULL;
    
    enc_ctx = &(devpair->transmission_keys_recv[key_index].aes_ctx);

    if (enc_ctx == NULL)
    {   
        al_log_error("Failed to get context.");
        return FAIL;
    }

    // Decrypt
    AES_CBC_decrypt_buffer(enc_ctx, data, size);

    return SUCCESS; 

};
