INTERFACE = wlp0s20f3
CC_BPF = clang-15
CC_SIM = gcc

default: sim

lib:
	$(CC_SIM) -O3 -g -DAL_SIM -I/usr/include/x86_64-linux-gnu -c ./src/al_lib.c -o ./bin/al_lib.o

sim:
	$(CC_SIM) -O3 -DAL_SIM ./thirdparty/tiny-AES-c/aes.c ./src/al_lib.c ./thirdparty/rfc6234/hkdf.o ./thirdparty/rfc6234/hmac.o ./thirdparty/rfc6234/usha.o ./thirdparty/rfc6234/sha384-512.o ./thirdparty/rfc6234/sha224-256.o ./thirdparty/rfc6234/sha1.o ./src/al_sim.c -o ./bin/al_sim

sim-device:
	$(CC_SIM) -O3 -DAL_SIM ./src/al_lib.c ./thirdparty/tiny-AES-c/aes.c ./thirdparty/rfc6234/hkdf.o ./thirdparty/rfc6234/hmac.o ./thirdparty/rfc6234/usha.o ./thirdparty/rfc6234/sha384-512.o ./thirdparty/rfc6234/sha224-256.o ./thirdparty/rfc6234/sha1.o ./src/al_sim_device.c -lzmq -o ./bin/al_sim_device 

sim-multi:
	$(CC_SIM) -DAL_SIM ./src/al_lib.c ./thirdparty/tiny-AES-c/aes.c ./thirdparty/rfc6234/hkdf.o ./thirdparty/rfc6234/hmac.o ./thirdparty/rfc6234/usha.o ./thirdparty/rfc6234/sha384-512.o ./thirdparty/rfc6234/sha224-256.o ./thirdparty/rfc6234/sha1.o ./src/al_sim_multi.c -lczmq -pg -o ./bin/al_sim_multi 

sim-proxy:
	$(CC_SIM) ./src/proxy.c -o ./bin/al_sim_proxy -lzmq

sim-upperlayer:
	$(CC_SIM) -O3 -DAL_SIM ./src/al_lib.c ./thirdparty/tiny-AES-c/aes.c ./thirdparty/rfc6234/hkdf.o ./thirdparty/rfc6234/hmac.o ./thirdparty/rfc6234/usha.o ./thirdparty/rfc6234/sha384-512.o ./thirdparty/rfc6234/sha224-256.o ./thirdparty/rfc6234/sha1.o ./src/al_sim_upperlayer.c -lczmq -lzmq -o ./bin/al_sim_upperlayer 

sim-upperlayer-multi:
	$(CC_SIM) -DAL_SIM ./src/al_lib.c ./thirdparty/tiny-AES-c/aes.c ./thirdparty/rfc6234/hkdf.o ./thirdparty/rfc6234/hmac.o ./thirdparty/rfc6234/usha.o ./thirdparty/rfc6234/sha384-512.o ./thirdparty/rfc6234/sha224-256.o ./thirdparty/rfc6234/sha1.o ./src/al_sim_upperlayer_multi.c -lczmq -lzmq -o ./bin/al_sim_upperlayer_multi 

clean:
	rm -f ./bin/*