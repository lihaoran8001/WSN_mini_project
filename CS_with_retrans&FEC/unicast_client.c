#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"

#include <string.h>
#include <stdio.h> /* For printf() */
#include "leds.h"
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "Client"
#define LOG_LEVEL LOG_LEVEL_INFO
#define RED_BLUE 0x50

/* Event config */
#define EVENT_SEND_MESSAGE 0x01
#define EVENT_TIMEOUT 0x02
#define EVENT_SEND_SUCCESS 0x03
#define EVENT_BUSY 0x04
#define EVENT_SEND_FAIL 0x05
#define MAX_RETRANSMISSION 3

/* message type config */
#define TYPE_ACK 0x10
#define TYPE_MSG 0x11

/* interval config */
#define SEND_INTERVAL (2 * CLOCK_SECOND)
#define SEND_TIMEOUT (0.15 * CLOCK_SECOND)
// kit108: 0x09, 0xe1, 0x93, 0x1c, 0x00, 0x74, 0x12, 0x00
// cooja: 0x01, 0x01, 0x01, 0x00, 0x01, 0x74, 0x12, 0x00
static linkaddr_t dest_addr = {{0x01, 0x01, 0x01, 0x00, 0x01, 0x74, 0x12, 0x00}};
static uint8_t msg_counter = 1;

#if MAC_CONF_WITH_TSCH
#include "net/mac/tsch/tsch.h"
static linkaddr_t coordinator_addr = {{0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
#endif /* MAC_CONF_WITH_TSCH */

static const uint8_t _hammingCalculateParityFast128[256]=
{ /* four-bits parity */
     0,  3,  5,  6,  6,  5,  3,  0,  7,  4,  2,  1,  1,  2,  4,  7,
     9, 10, 12, 15, 15, 12, 10,  9, 14, 13, 11,  8,  8, 11, 13, 14,
    10,  9, 15, 12, 12, 15,  9, 10, 13, 14,  8, 11, 11,  8, 14, 13,
     3,  0,  6,  5,  5,  6,  0,  3,  4,  7,  1,  2,  2,  1,  7,  4,
    11,  8, 14, 13, 13, 14,  8, 11, 12, 15,  9, 10, 10,  9, 15, 12,
     2,  1,  7,  4,  4,  7,  1,  2,  5,  6,  0,  3,  3,  0,  6,  5,
     1,  2,  4,  7,  7,  4,  2,  1,  6,  5,  3,  0,  0,  3,  5,  6,
     8, 11, 13, 14, 14, 13, 11,  8, 15, 12, 10,  9,  9, 10, 12, 15,
    12, 15,  9, 10, 10,  9, 15, 12, 11,  8, 14, 13, 13, 14,  8, 11,
     5,  6,  0,  3,  3,  0,  6,  5,  2,  1,  7,  4,  4,  7,  1,  2,
     6,  5,  3,  0,  0,  3,  5,  6,  1,  2,  4,  7,  7,  4,  2,  1,
    15, 12, 10,  9,  9, 10, 12, 15,  8, 11, 13, 14, 14, 13, 11,  8,
     7,  4,  2,  1,  1,  2,  4,  7,  0,  3,  5,  6,  6,  5,  3,  0,
    14, 13, 11,  8,  8, 11, 13, 14,  9, 10, 12, 15, 15, 12, 10,  9,
    13, 14,  8, 11, 11,  8, 14, 13, 10,  9, 15, 12, 12, 15,  9, 10,
     4,  7,  1,  2,  2,  1,  7,  4,  3,  0,  6,  5,  5,  6,  0,  3,
}; 

// Given two bytes to transmit, this returns the parity
// as a byte with the lower nibble being for the first byte,
// and the upper nibble being for the second byte.
uint8_t DL_HammingCalculateParity2416(uint8_t first, uint8_t second)
{
    return (_hammingCalculateParityFast128[second]<<4) | _hammingCalculateParityFast128[first];
}


static uint8_t encode_packet(uint8_t* msgin, uint8_t len, uint8_t* msgout){
  if(len % 2){
    msgin[len] = 0xff;
  }
  int i, j;
  uint8_t b1, b2, h;
  for(i = 0, j = 0; i < len; i+=2){
    b1 = msgin[i]; 
    b2 = msgin[i+1];
    
    h = DL_HammingCalculateParity2416(b1, b2);
    // LOG_INFO("adding %d:%.02x,%.02x parity:%.02x\n", i, b1, b2, h);
    msgout[j++] = b1;
    msgout[j++] = b2;
    msgout[j++] = h;
    // LOG_INFO("length: %d\n", j);
  }

  return 0;
}

/*---------------------------------------------------------------------------*/
PROCESS(nullnet_example_process, "NullNet unicast example");
PROCESS(send_msg_process, "send_msg process");
AUTOSTART_PROCESSES(&nullnet_example_process, &send_msg_process);
/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len,
                    const linkaddr_t *src, const linkaddr_t *dest)
{
    uint8_t type = *(uint8_t*)data;
    uint8_t seq = *(uint8_t*)(data+1);
    switch(type){
        case TYPE_ACK:
            LOG_INFO("client receive ACK of packet No.%02x\n", seq);
            process_post(&send_msg_process, EVENT_SEND_SUCCESS, NULL);
            break;

        default:
            LOG_INFO("Not recognized response\n");
            break;
    }
}

static void time_out_callback(void *ptr)
{
  LOG_INFO("Timeout message NO.%02x \n", *(uint8_t*)ptr);
  process_post(&send_msg_process, EVENT_TIMEOUT, NULL);
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(send_msg_process, ev, data)
{
    static char str_buf[128];
    static char encode_buf[256];
    static struct ctimer to_timer;
    static int rt_counter;
    static int flag;
    PROCESS_BEGIN();
    while(1){
        PROCESS_WAIT_EVENT(); 
        if(ev == EVENT_SEND_MESSAGE)
        {
            // clean buffer
            bzero(str_buf, 128);
            bzero(encode_buf, 256);
            // append message type and message counter on top of the buffer
            str_buf[0] = TYPE_MSG;
            str_buf[1] = msg_counter;
            strcat(str_buf, (char*)data);
            // generate error correction codes for str_buf
            // LOG_INFO("original length is %d...\n", strlen(data));
            encode_packet((uint8_t*)str_buf, strlen(str_buf), (uint8_t*)encode_buf);
            nullnet_buf = (uint8_t *)encode_buf;
            nullnet_len = strlen(encode_buf);
            // LOG_INFO("sending length is %d...\n", nullnet_len);
            nullnet_set_input_callback(input_callback);
            leds_toggle(RED_BLUE);
            flag = 0;
            rt_counter = 1;
            while(rt_counter <= MAX_RETRANSMISSION){
                LOG_INFO("Sending message NO.%02x attempt %d...\n", msg_counter, rt_counter);
                NETSTACK_NETWORK.output(&dest_addr); // sending function
                ctimer_set(&to_timer, SEND_TIMEOUT, time_out_callback, &msg_counter);
                PROCESS_WAIT_EVENT();
                if(ev == EVENT_TIMEOUT){
                    rt_counter += 1;
                    continue;
                }
                else if(ev == EVENT_SEND_SUCCESS){
                    // send success
                    // stop timer and continue to wait for next message
                    ctimer_stop(&to_timer);
                    flag = 1;
                    break;
                }
                else{
                    LOG_INFO("Unknown Event\n");
                }
                
            }

            if(flag == 1){
                LOG_INFO("Message No.%02x send success!\n", msg_counter);
                process_post(&nullnet_example_process, EVENT_SEND_SUCCESS, NULL);
            }else{
                LOG_INFO("Message No.%02x send fail!\n", msg_counter);
                process_post(&nullnet_example_process, EVENT_SEND_FAIL, NULL);
            }
            msg_counter += 1;
        }
    }

    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(nullnet_example_process, ev, data)
{
    static struct etimer periodic_timer;
    static char str[64]; // plaintext
    leds_init();
    leds_on(RED_BLUE);
    PROCESS_BEGIN();

    #if MAC_CONF_WITH_TSCH
    tsch_set_coordinator(linkaddr_cmp(&coordinator_addr, &linkaddr_node_addr));
    #endif /* MAC_CONF_WITH_TSCH */


    strcpy(str, "user:TeloSwift; pwd:1234567890;"); // plaintext is "user:TeloSwift; pwd:1234567890;"

    etimer_set(&periodic_timer, SEND_INTERVAL);
    while (1)
    {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
        leds_toggle(RED_BLUE);
        process_post(&send_msg_process, EVENT_SEND_MESSAGE, str); // sending function
        PROCESS_WAIT_EVENT();
        if(ev == EVENT_SEND_FAIL){
            LOG_INFO("adjust longer sending interval!\n");
            etimer_reset_with_new_interval(&periodic_timer, 5*SEND_INTERVAL);
        }else{
            etimer_reset_with_new_interval(&periodic_timer, SEND_INTERVAL);
        }
    }
    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
