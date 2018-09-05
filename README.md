# garpd

garpd is a toy gratuitous arp generator I built to learn about raw AF_PACKET sockets in Linux. 
It formats its own Ethernet and ARP headers into an array of bytes and sends it across the wire.
A simple loop in the main routine sends a gratuitous reply every x second interval.

## Install and Run
```
# make && make install
# bin/garpd "enp0s3" "192.168.1.66"
```
