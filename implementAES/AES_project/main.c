/**
 * @ Author:    Kong Weize <202101694@post.au.dk>
 * @ Date:      18.Mar 2022
 */

#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "lib/aes-128.h"
#include "sys/log.h"

#define LOG_MODULE "Clt"
#define LOG_LEVEL LOG_LEVEL_INFO

/*---------------------------------------------------------------------------*/
PROCESS(channel_sensing, "Channel sensing process");
AUTOSTART_PROCESSES(&channel_sensing);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(channel_sensing, ev, data)
{
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

    static struct timer timer;
    PROCESS_BEGIN();

    AES_128.set_key(key);

    // AES_128.encrypt(plaintext);
    AES_128.encrypt_cbc(plaintext, key, iv, 16);

    for (i = 0; i < 16; i++)
    {
        printf("%x ", plaintext[i]);
    }

    printf("\n");
    while (1)
    {
        LOG_INFO("running");
        LOG_INFO_("\n");
        timer_set(&timer, CLOCK_SECOND * 5);
        while (!timer_expired(&timer))
        {
        }
        // printf("running\n");
        timer_reset(&timer);
    }
    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
