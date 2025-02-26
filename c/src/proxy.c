#include <zmq.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "al_lib.h"

static void listener_thread(void *args, void *pipe)
{
    ALPacket packet = {0};
    //  Print everything that arrives on pipe
    while (1) {
        zmq_recv(pipe, &packet, sizeof(packet), 0);
        // print_alpacket_all(&packet, SMALL_MTU);
        printf("received: %s\n", (char*)&packet);
    }
}

int main (void)
{

    // Create connection address strings for BIND calls.
    char broadcast_sock_address_in[sizeof(AL_DOMAIN_BIND) + 1 + 5 + 1] = {0}; //  + ":" + max port length of 5 + null.
    snprintf(broadcast_sock_address_in, sizeof(broadcast_sock_address_in), "%s:%d", AL_DOMAIN_BIND, AL_PROXY_PORT_IN);
    char broadcast_sock_address_out[sizeof(AL_DOMAIN_BIND) + 1 + 5 + 1] = {0}; //  + ":" + max port length of 5 + null.
    snprintf(broadcast_sock_address_out, sizeof(broadcast_sock_address_out), "%s:%d", AL_DOMAIN_BIND, AL_PROXY_PORT_OUT);

    printf("Creating backend that publishers connect to with %s\n", broadcast_sock_address_in);
    printf("Creating frontend that subscribers connect to with %s\n", broadcast_sock_address_out);

    // Create ZMQ context
    void *context = zmq_ctx_new();

    // Create the proxy. Backend is a xsub which publishers push to. Frontend is a xpub that subscribers pull from..
    void *backend = zmq_socket(context, ZMQ_XSUB);
    zmq_bind(backend, broadcast_sock_address_in);
    sleep(1);

    void *frontend = zmq_socket(context, ZMQ_XPUB);
    zmq_bind(frontend, broadcast_sock_address_out);
    sleep(1);

    // Create a listener, optional to monitor incoming data
    // void* capture = zmq_socket(context, ZMQ_PUB);
    // zmq_connect(capture, broadcast_sock_address_out);

    // Start the proxy
    printf("Starting proxy.\n");
    zmq_proxy(frontend, backend, NULL);

    // Clean up
    zmq_close(frontend);
    zmq_close(backend);
    zmq_ctx_destroy(context);

    return 0;
}