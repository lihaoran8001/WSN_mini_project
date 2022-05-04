

#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "sys/log.h"
#include "leds.h"

#include "lib/aes-128.h"

#include <string.h>
#include <stdio.h> /* For printf() */

/* Log configuration */

#define LOG_MODULE "Client"
#define LOG_LEVEL LOG_LEVEL_INFO
#define RED_BLUE 0x50
/* Configuration */
#define SEND_INTERVAL (4 * CLOCK_SECOND)

// kit108: 0x09, 0xe1, 0x93, 0x1c, 0x00, 0x74, 0x12, 0x00
// cooja: 0x01, 0x01, 0x01, 0x00, 0x01, 0x74, 0x12, 0x00
static linkaddr_t dest_addr = {{0x09, 0xe1, 0x93, 0x1c, 0x00, 0x74, 0x12, 0x00}};

#if MAC_CONF_WITH_TSCH
#include "net/mac/tsch/tsch.h"
static linkaddr_t coordinator_addr = {{0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
#endif /* MAC_CONF_WITH_TSCH */

/*---------------------------------------------------------------------------*/
PROCESS(nullnet_example_process, "NullNet unicast example");
AUTOSTART_PROCESSES(&nullnet_example_process);

/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len,
                    const linkaddr_t *src, const linkaddr_t *dest)
{
  LOG_INFO("Received response '%s'", (char *)data);
  LOG_INFO_("\n");
}

static void
encrypt_cbc(uint8_t *in, uint8_t *key, uint8_t *iv, unsigned long size)
{
  int blocks = size / 16;
  int i, j, k;
  for (k = 0; k < blocks; k++)
  {
    j = k * 16;
    for (i = 0; i < 16; i++)
    {
      in[i + j] = in[i + j] ^ iv[i];
    }
    AES_128.encrypt(in + j);
    iv = in + j;
  }
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(nullnet_example_process, ev, data)
{
  static struct etimer periodic_timer;
  static char str[64]; // plaintext to be sent

  //----------------------------init key, plaintext, iv
  uint8_t key[16];
  uint8_t plaintext[16];
  uint8_t iv[16];

  int i;
  for (i = 0; i < 16; i++)
  {
    key[i] = '2';
    plaintext[i] = '1';
    iv[i] = '3';
  }
  leds_init();
  leds_on(RED_BLUE);
  PROCESS_BEGIN();

#if MAC_CONF_WITH_TSCH
  tsch_set_coordinator(linkaddr_cmp(&coordinator_addr, &linkaddr_node_addr));
#endif /* MAC_CONF_WITH_TSCH */

  /* Initialize NullNet */
  // strcpy(str, "user:TeloSwift; pwd:1234567890;"); // plaintext is "user:TeloSwift; pwd:1234567890;"

  //------------------encrypt here and measure time-----------------------

  AES_128.set_key(key);
  // AES_128.encrypt(plaintext); // ECB mode
  encrypt_cbc(plaintext, key, iv, 16); // CBC mode

  for (i = 0; i < 16; i++)
  {
    printf("%x", plaintext[i]);
  }
  printf("\n");
  strcpy(str, (char *)plaintext);

  //--------------configure nullnet--------------
  nullnet_buf = (uint8_t *)str;
  // nullnet_len = strlen(str);
  // nullnet_buf = (uint8_t *)plaintext;
  nullnet_len = 16; // lenth of 'plaintext' instead of 'str', otherwise it might be some bugs.

  nullnet_set_input_callback(input_callback);

  if (!linkaddr_cmp(&dest_addr, &linkaddr_node_addr))
  {
    etimer_set(&periodic_timer, SEND_INTERVAL);

    while (1)
    {
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
      leds_toggle(RED_BLUE);
      LOG_INFO("Sending");
      LOG_INFO_LLADDR(&dest_addr);
      LOG_INFO_("\n");

      // unsigned long t_start = clock_seconds();
      // //printf("Time to start! %lu \n", t_start);

      NETSTACK_NETWORK.output(&dest_addr); // sending function

      // unsigned long t_end = clock_seconds();
      // printf("Finishing sending %lu \n", t_end);

      etimer_reset(&periodic_timer);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
