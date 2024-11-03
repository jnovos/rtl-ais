/*
 *    main.cpp  --  AIS Decoder
 *
 *    Copyright (C) 2013
 *      Astra Paging Ltd / AISHub (info@aishub.net)
 *
 *    AISDecoder is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    AISDecoder uses parts of GNUAIS project (http://gnuais.sourceforge.net/)
 *
 */
/* This is a stripped down version for use with rtl_ais*/

#define _GNU_SOURCE
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <pthread.h>
// #include "config.h"
#include "sounddecoder.h"
#include "lib/callbacks.h"
#include "../tcp_listener/tcp_listener.h"

#define MAX_BUFFER_LENGTH 2048
// #define MAX_BUFFER_LENGTH 8190

static char buffer[MAX_BUFFER_LENGTH];
static unsigned int buffer_count = 0;
static int _debug_nmea;
static int _debug;
static int sock;
static int _use_tcp;

static struct addrinfo *addr = NULL;
// messages can be retrived from a different thread
static pthread_mutex_t message_mutex;

// queue of decoded ais messages
struct ais_message
{
    char *buffer;
    struct ais_message *next;
} *ais_messages_head, *ais_messages_tail, *last_message;

static void append_message(const char *buffer)
{
    struct ais_message *m = malloc(sizeof *m);
    m->buffer = strdup(buffer);
    m->next = NULL;
    pthread_mutex_lock(&message_mutex);
    // enqueue
    if (!ais_messages_head)
        ais_messages_head = m;
    else
        ais_messages_tail->next = m;
    ais_messages_tail = m;
    pthread_mutex_unlock(&message_mutex);
}

static void free_message(struct ais_message *m)
{
    if (m)
    {
        free(m->buffer);
        free(m);
    }
}

const char *aisdecoder_next_message()
{
    free_message(last_message);
    last_message = NULL;

    pthread_mutex_lock(&message_mutex);
    if (!ais_messages_head)
    {
        pthread_mutex_unlock(&message_mutex);
        return NULL;
    }

    // dequeue
    last_message = ais_messages_head;
    ais_messages_head = ais_messages_head->next;

    pthread_mutex_unlock(&message_mutex);
    return last_message->buffer;
}

static int initSocket(const char *host, const char *portname);
int send_nmea(const char *sentence, unsigned int length);

void sound_level_changed(float level, int channel, unsigned char high)
{
    if (high != 0)
        fprintf(stderr, "Level on ch %d too high: %.0f %%\n", channel, level);
    else
        fprintf(stderr, "Level on ch %d: %.0f %%\n", channel, level);
}

void nmea_sentence_received(const char *sentence,
                            unsigned int length,
                            unsigned char sentences,
                            unsigned char sentencenum)
{
    append_message(sentence);
    if (sentences == 1)
    {
        if (send_nmea(sentence, length) == -1)
        {
            if (_debug)
                fprintf(stderr, "-----Send_nmea Abort....");
            abort();
        }
        if (_debug)
            fprintf(stderr, "---%s", sentence);
    }
    else
    {
        if (buffer_count + length < MAX_BUFFER_LENGTH)
        {
            memcpy(&buffer[buffer_count], sentence, length);
            buffer_count += length;
        }
        else
        {
            buffer_count = 0;
        }

        if (sentences == sentencenum && buffer_count > 0)
        {
            if (send_nmea(buffer, buffer_count) == -1)
            {
                if (_debug)
                    fprintf(stderr, "*****Send_nmea Abort....");
                abort();
            }
            if (_debug)
                fprintf(stderr, "******%s", buffer);
            buffer_count = 0;
        };
    }
}

int send_nmea(const char *sentence, unsigned int length)
{
    if (_use_tcp)
    {
        return add_nmea_ais_message(sentence, length);
    }
    else if (sock)
    {
        return sendto(sock, sentence, length, 0, addr->ai_addr, addr->ai_addrlen);
    }
    return 0;
}

int init_ais_decoder(char *host, char *port, int show_levels, int debug_nmea, int buf_len, int time_print_stats, int use_tcp_listener, int tcp_keep_ais_time, int tcp_stream_forever, int add_sample_num, unsigned long mmsi,int debug)
{
    _debug_nmea = debug_nmea;
    _debug = debug;
    _use_tcp = use_tcp_listener;
    pthread_mutex_init(&message_mutex, NULL);
    if (_debug)
        fprintf(stderr, "Log to console ON\n");
    if (_debug_nmea)
        fprintf(stderr, "Log NMEA sentences to console ON\n");
    else
        fprintf(stderr, "Log NMEA sentences to console OFF\n");
    if (!_use_tcp)
    {
        fprintf(stderr, "Send NMEA sentences to UDP ON\n");
        if (host && port && !initSocket(host, port))
        {
            fprintf(stderr, "Error to InitSocketto %s port %s\n", host, port);
            return EXIT_FAILURE;
        }
    }
    else
    {
        fprintf(stderr, "Send NMEA sentences to TCP ON\n");
        if (!initTcpSocket(port, debug, tcp_keep_ais_time, tcp_stream_forever))
        {
            fprintf(stderr, "Error to initTcpSocket %s port %s\n", host, port);
            return EXIT_FAILURE;
        }
    }
    if (show_levels)
        on_sound_level_changed = sound_level_changed;
    on_nmea_sentence_received = nmea_sentence_received;
    initSoundDecoder(buf_len, time_print_stats, add_sample_num, mmsi);
    return 0;
}

void run_rtlais_decoder(short *buff, int len)
{
    run_mem_decoder(buff, len, MAX_BUFFER_LENGTH);
}
int free_ais_decoder(void)
{
    pthread_mutex_destroy(&message_mutex);

    // free all stored messa ages
    free_message(last_message);
    last_message = NULL;

    while (ais_messages_head)
    {
        struct ais_message *m = ais_messages_head;
        ais_messages_head = ais_messages_head->next;
        free_message(m);
    }

    freeSoundDecoder();
    freeaddrinfo(addr);
    return 0;
}

int initSocket(const char *host, const char *portname)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    int err = getaddrinfo(host, portname, &hints, &addr);
    if (err != 0)
    {
        fprintf(stderr, "Failed to resolve remote socket address!\n");
        return 0;
    }
    sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if (sock == -1)
    {
        fprintf(stderr, "%s", strerror(errno));
        return 0;
    }
     if (_debug_nmea)
        fprintf(stderr, "AIS data will be sent to %s port %s\n", host, portname);

    return 1;
}
