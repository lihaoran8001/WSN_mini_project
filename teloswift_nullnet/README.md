# TeloSwift Team 10

chooes hardware or software encryption, please configure in 'project-conf.h'  
chooes CBC or ECB mode, please modify 'unicast_client.c' line 14.  

This demo achieve communication between two nodes(only two because of unicast) on Link layer, that is, the only things need to know is the Link layer address.

## Client
file: unicast_client.c
Client needs to pre-know the link layer address of the server and encrypt and send the cipher text to server by `NETSTACK_NETWORK.output(&dest_addr);`

The only thing you need to do is to change the `dest_addr` to the address of your device. For example, the address of my mote kit:108 is `09e1.931c.0074.1200`. 
Also, you can run on cooja environment but it still needs to set proper `dest_addr`

## Server
file: unicast_server.c
Server can get the cipher text by call back function `input_callback();`

