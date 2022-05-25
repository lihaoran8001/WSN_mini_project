
#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"

#include <string.h>
#include <stdio.h> /* For printf() */

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "Server"
#define LOG_LEVEL LOG_LEVEL_INFO

/* Configuration */
#define SEND_INTERVAL (4 * CLOCK_SECOND)
#define TYPE_ACK 0x10
#define TYPE_MSG 0x11
// kit108: 0012.7400.1c93.e109
// cooja: 0x01, 0x01, 0x01, 0x00, 0x01, 0x74, 0x12, 0x00
// static linkaddr_t dest_addr = {{0x00, 0x12, 0x74, 0x00, 0x1c, 0x93, 0xe1, 0x09}};

#if MAC_CONF_WITH_TSCH
#include "net/mac/tsch/tsch.h"
static linkaddr_t coordinator_addr = {{0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
#endif /* MAC_CONF_WITH_TSCH */

/*---------------------------------------------------------------------------*/
PROCESS(nullnet_example_process, "NullNet unicast example");
AUTOSTART_PROCESSES(&nullnet_example_process);

/*---------------------------------------------------------------------------*/

// This contains all of the precalculated parity values for a byte (8 bits).
// This is very fast, but takes up more program space than calculating on the fly.
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

#define UNCORRECTABLE   0xFF
#define ERROR_IN_PARITY 0xFE
#define NO_ERROR        0x00

// Private table. Faster and more compact than multiple if statements.
static const uint8_t _hammingCorrect128Syndrome[16] =
{
    NO_ERROR,           // 0
    ERROR_IN_PARITY,    // 1
    ERROR_IN_PARITY,    // 2
    0x01,               // 3
    ERROR_IN_PARITY,    // 4
    0x02,               // 5
    0x04,               // 6
    0x08,               // 7
    ERROR_IN_PARITY,    // 8
    0x10,               // 9
    0x20,               // 10
    0x40,               // 11
    0x80,               // 12
    UNCORRECTABLE,      // 13
    UNCORRECTABLE,      // 14
    UNCORRECTABLE,      // 15
};

// Private method
// Give a pointer to a received byte,
// and given a nibble difference in parity (parity ^ calculated parity)
// this will correct the received byte value if possible.
// It returns the number of bits corrected:
// 0 means no errors
// 0 means one corrected error
// 1 means corrections not possible
static uint8_t DL_HammingCorrect128Syndrome(uint8_t* value, uint8_t syndrome)
{
    // Using only the lower nibble (& 0x0F), look up the bit
    // to correct in a table
    uint8_t correction = _hammingCorrect128Syndrome[syndrome & 0x0F];

    if (correction != NO_ERROR){
        if (correction == UNCORRECTABLE || value == NULL){
            return 1; // Non-recoverable error
        } else {
            if (correction != ERROR_IN_PARITY){
                *value ^= correction;
            }
            return 0; // 1-bit recoverable error;
        }
    }
    return 0; // No errors
}

// Given a pointer to a first value and a pointer to a second value and
// their combined given parity (lower nibble first parity, upper nibble second parity),
// this calculates what the parity should be and fixes the values if needed.
// It returns the number of bits corrected:
// 0 means no errors
// 0 means one corrected error
// 0 means two corrected errors
// 1 means corrections not possible
uint8_t DL_HammingCorrect2416(uint8_t* first, uint8_t* second, uint8_t parity)
{
    if (first == NULL || second == NULL){
        return 1; // Non-recoverable error
    }

    uint8_t syndrome = DL_HammingCalculateParity2416(*first, *second) ^ parity;

    if (syndrome != 0){
        return DL_HammingCorrect128Syndrome(first, syndrome) + DL_HammingCorrect128Syndrome(second, syndrome >> 4);
    }

    return 0; // No errors
}

/* return number of errors + 128 if CRC error */
static uint8_t decode_packet(uint8_t* msgin, uint16_t olen, uint8_t* msgout){
  int i=0, j=0;
  uint8_t b1, b2, h, c = 0;

  for(i = 0, j = 0; i < olen - 2; i+=3){
    b1 = msgin[i]; b2 = msgin[i+1]; h = msgin[i+2];
    c += DL_HammingCorrect2416(&b1, &b2, h);
    msgout[j++] = b1;
    msgout[j++] = b2;
  }
  j--;
  if(msgout[j] == 0xff){
    msgout[j] = 0x00;
  }
  return c;
}

void input_callback(const void *data, uint16_t len,
                    const linkaddr_t *src, const linkaddr_t *dest)
{
    // LOG_INFO("Received Raw Message length:%d\n", (int)len);
    static char str_buf[128];
    bzero(str_buf, 128);
    decode_packet((uint8_t*)data, len, (uint8_t*)str_buf);
    // uint8_t type = *(uint8_t*) str_buf;
    uint8_t seq = *(uint8_t*) (str_buf+1);
    // uint8_t* payload = (uint8_t*) (str_buf+2);
    // LOG_INFO("Received Message type:%02x seq:%02x payload:%s \n", type, seq, payload);
    // int i = 0;
    // printf("Received payload:");
    // for (i = 0; i < 16; i++)
    // {
    //     printf("%x ", payload[i]);
    // }
    // printf("\n");
    char response[3];
    response[0] = TYPE_ACK;
    response[1] = seq;
    response[2] = 0x00;
    nullnet_buf = (uint8_t*)response;
    nullnet_len = strlen(response);
    NETSTACK_NETWORK.output(src);
    // LOG_INFO("send back type:%02x seq:%02x \n", (uint8_t)response[0],(uint8_t)response[1]);
    
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(nullnet_example_process, ev, data)
{

    PROCESS_BEGIN();

#if MAC_CONF_WITH_TSCH
    tsch_set_coordinator(linkaddr_cmp(&coordinator_addr, &linkaddr_node_addr));
#endif /* MAC_CONF_WITH_TSCH */
    LOG_INFO("Server started!\n");
    nullnet_set_input_callback(input_callback);

    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
