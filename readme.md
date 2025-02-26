# Anonymization Layer

Anonymization Layer (AL) is a transparent, protocol agnostic layer to further enhance the privacy of network communication. It achieves this by removing data flow directionality, hiding message content and packet context, and providing tunable pseudo responses. Specifically, AL was developed specifically as a defense against association-based inference attacks, such as IDBleed on exclusive-use communication patterns.

For more detailed information, please explore our paper ["Deanonymizing Device Identities via Side-channel Attacks in Exclusive-use IoTs & Mitigation"](https://www.ndss-symposium.org/wp-content/uploads/2025-703-paper.pdf) which was published in Network & Distributed System Security (NDSS) 2025. AL was developed by the first author, Christopher Ellis.

## Building

This repository contains the core components of AL for demonstration and evaluation, implemented in C. There is a simplistic make file that allows compilation of the following binaries:

- al_sim: a standalone demonstration of basic functionality in a single file.
- al_sim_device: a singular "device" that takes on an ID and operates as an AL layer, receiving input from a higher(upper) layer.
- al_sim_upperlayer: creates random data and sends it down to an al_sim_device by id, simulating a layer 2 packet.
- al_sim_multi: a multi-instance al_sim_device launcher that can be manually configured inline.
- al_sim_upperlayer_multi: creates threads to send data to multiple devices to simulate a congested environment.
- al_sim_proxy: a simple proxy that acts as the pub/sub proxy to simulate a broadcast channel

This prototype uses third-party software, particularly for encryption and zeromq to mainly simulate a broadcast channel.  Some files require configuration to evaluate specific scenarios, either in the C source file itself, or the al_lib.h. Care was taken to make it configurable without magic numbers throughout the code. A revised version will provide proper arg parsing for enhanced usability.

Note, you may need to install the zeromq dev packages for your particular distribution. This was developed and evaluated using Ubuntu 22.04.

## Example Usage

In separate terminals...

1. Run al_sim_proxy.
2. Run al_sim_device twice, with different parameters. In order, these are:
    - ID for this device
    - Number of devices to connect to
    - Starting device ID to create pairs from

For example, the following will create a device with ID 1, create pairs to 3 devices, starting at device ID 2.

```$ ./al_sim_device 1 3 2```

Then create another device with ID 2 to communicate with device 1, such as:

```$ ./al_sim_device 2 1 1```

3. Run al_sim_upperlayer, using parameters:
    - Number of packets to send
    - Source Device ID
    - Destination Device ID

Such as:

```$ ./al_sim_upperlayer 10 1 2```

Each terminal should now display output to demonstrate the creation, receipt from upper layer, encapsulation in as an AL packet, transmission, then receipt and the unwinding by the destination device. Note, there are various levels of output, and is only recommended to prove in a demo. When evaluating performance, limit output.