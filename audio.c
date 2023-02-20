//*****************
//* audio.c
//* rev 1.0 July 2018, Shabaz
//* Free for non-commercial use
//*
//*****************

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "wav.h"

#define LINUX

#define MAXVOL 32767
// Envelope profile(s)
#define ENV_ADSR 1
#define PHASE_ATTACK 1
#define PHASE_DECAY 2
#define PHASE_SUSTAIN 3
#define PHASE_RELEASE 4
#define PHASE_COMPLETE 99

#define PAUSE 90

#define OPER_0 0
#define OPER_1 1
#define OPER_2 2
#define OPER_3 3

#define OSC_INIT 0
#define FREQ_UPDATE 1
#define SAMPLE_RATE                     16000
#define FRAME_SIZE                      80

typedef struct inst_param_s {
	uint16_t amp;
	int16_t dur_recip10;
	int16_t carrfreq;
	int16_t dev1;
	int16_t dev2;
	uint16_t modfreq10;
} inst_param_t;	

typedef struct osc_s {
	uint16_t freq10;
  uint16_t freq;
  uint16_t phase;
  int16_t amplitude;
  uint16_t lowsamp;
  uint16_t m;
  char env;
  char env_phase;
  int16_t outbuf;

  uint16_t idx;
  uint16_t incr;
} osc_t;

static osc_t osc[4];
static inst_param_t inst;
PCM16_stereo_t  *buffer_p = NULL; // used for writing to wav file

uint16_t buf0[FRAME_SIZE];

// this table starts at note C1 (i.e. assume this is the leftmost key on the simulated music keyboard)
const unsigned int keyfreq[61]={
	65, 69, 73, 78, 82, 87, 92, 98, 104, 110, 117, 123,
	131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247,
	262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, /* c3 */
	523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988,
	1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976,
  2093 };


const unsigned int poly1keys[]={
PAUSE, PAUSE, PAUSE, PAUSE, PAUSE, 24, 23,   21, 17, 17, 12, 17, 17,   21, 18, 21, 24, 23, 21, 
23, 19, 19, 14, 19, 19,   23, 19, 23, 26, 24, 23,   21, 17, 17, 12, 17, 17,

21, 17, 21, 24, 23, 21,   23, 21, 23, 19, 24, 23,   21, 17, 17, 17,

/* repeat: */
24, 23,   21, 17, 17, 12, 17, 17,   21, 18, 21, 24, 23, 21, 
23, 19, 19, 14, 19, 19,   23, 19, 23, 26, 24, 23,   21, 17, 17, 12, 17, 17,

21, 17, 21, 24, 23, 21,   23, 21, 23, 19, 24, 23,   21, 17, 17, 17,

/* next section: */
29,     29, 24, 29, 29, 24, 29,   29, 24, 29, 33, 31, 29,
28, 24, 28, 28, 24, 28,    28, 24, 28, 31, 29, 28,    26, 29, 29, 24, 29, 29,
23, 29, 29, 21, 29, 29,    23, 21, 23, 19, 24, 23,    21, 17, 17, 17,
/* repeat: */
29,     29, 24, 29, 29, 24, 29,   29, 24, 29, 33, 31, 29,
28, 24, 28, 28, 24, 28,    28, 24, 28, 31, 29, 28,    26, 29, 29, 24, 29, 29,
23, 29, 29, 21, 29, 29,    23, 21, 23, 19, 24, 23,    21, 17, 17, 17,


99};
const unsigned int poly1periods[]={
4, 4, 4, 4, 4, 2, 2,     4,  4,  4,  4,  4,  4,    4,  4,  4,  4,  4,  4,  
4,  4,  4,  4,  4,  4,    4,  4,  4,  4,  4,  4,    4,  4,  4,  4,  4,  4, 
4,  4,  4,  4,  4,  4,    4,  4,  4,  4,  4,  4,    4,  4,  4,  8,

/* repeat: */
2, 2,     4,  4,  4,  4,  4,  4,    4,  4,  4,  4,  4,  4,  
4,  4,  4,  4,  4,  4,    4,  4,  4,  4,  4,  4,    4,  4,  4,  4,  4,  4, 
4,  4,  4,  4,  4,  4,    4,  4,  4,  4,  4,  4,    4,  4,  4,  8,

/* next section: */
4,      4,  4,  4,  4,  4,  4,    4,  4,  4,  4,  4,  4,
4,  4,  4,  4,  4,  4,      4,  4,  4,  4,  4,  4,      4,  4,  4,  4,  4,  4,
4,  4,  4,  4,  4,  4,      4,  4,  4,  4,  4,  4,    4,  4,  4, 8, 
/* repeat: */
4,      4,  4,  4,  4,  4,  4,    4,  4,  4,  4,  4,  4,
4,  4,  4,  4,  4,  4,      4,  4,  4,  4,  4,  4,      4,  4,  4,  4,  4,  4,
4,  4,  4,  4,  4,  4,      4,  4,  4,  4,  4,  4,    4,  4,  4, 8, 


99};


const int16_t sinearr[1000] = {
    0x0000, 0x00ce, 0x019c, 0x026a, 0x0337, 0x0405, 0x04d3, 0x05a1, 0x066e, 0x073c,
    0x080a, 0x08d7, 0x09a4, 0x0a72, 0x0b3f, 0x0c0c, 0x0cd9, 0x0da5, 0x0e72, 0x0f3f,
    0x100b, 0x10d7, 0x11a3, 0x126f, 0x133b, 0x1406, 0x14d1, 0x159c, 0x1667, 0x1732,
    0x17fc, 0x18c6, 0x1990, 0x1a5a, 0x1b23, 0x1bec, 0x1cb5, 0x1d7d, 0x1e46, 0x1f0d,
    0x1fd5, 0x209c, 0x2163, 0x222a, 0x22f0, 0x23b6, 0x247c, 0x2541, 0x2605, 0x26ca,
    0x278e, 0x2851, 0x2915, 0x29d7, 0x2a9a, 0x2b5c, 0x2c1d, 0x2cde, 0x2d9f, 0x2e5f,
    0x2f1f, 0x2fde, 0x309d, 0x315b, 0x3219, 0x32d6, 0x3392, 0x344f, 0x350a, 0x35c5,
    0x3680, 0x373a, 0x37f3, 0x38ac, 0x3965, 0x3a1c, 0x3ad4, 0x3b8a, 0x3c40, 0x3cf5,
    0x3daa, 0x3e5e, 0x3f12, 0x3fc5, 0x4077, 0x4128, 0x41d9, 0x4289, 0x4339, 0x43e8,
    0x4496, 0x4543, 0x45f0, 0x469c, 0x4748, 0x47f2, 0x489c, 0x4945, 0x49ee, 0x4a96,
    0x4b3d, 0x4be3, 0x4c88, 0x4d2d, 0x4dd1, 0x4e74, 0x4f16, 0x4fb8, 0x5058, 0x50f8,
    0x5197, 0x5235, 0x52d3, 0x536f, 0x540b, 0x54a6, 0x5540, 0x55d9, 0x5671, 0x5709,
    0x579f, 0x5835, 0x58ca, 0x595d, 0x59f0, 0x5a82, 0x5b14, 0x5ba4, 0x5c33, 0x5cc1,
    0x5d4f, 0x5ddb, 0x5e67, 0x5ef1, 0x5f7b, 0x6004, 0x608b, 0x6112, 0x6198, 0x621c,
    0x62a0, 0x6323, 0x63a5, 0x6425, 0x64a5, 0x6524, 0x65a1, 0x661e, 0x669a, 0x6714,
    0x678e, 0x6806, 0x687e, 0x68f4, 0x696a, 0x69de, 0x6a51, 0x6ac3, 0x6b34, 0x6ba4,
    0x6c13, 0x6c81, 0x6ced, 0x6d59, 0x6dc3, 0x6e2d, 0x6e95, 0x6efc, 0x6f62, 0x6fc7,
    0x702b, 0x708d, 0x70ef, 0x714f, 0x71ae, 0x720d, 0x7269, 0x72c5, 0x7320, 0x7379,
    0x73d1, 0x7428, 0x747e, 0x74d3, 0x7527, 0x7579, 0x75ca, 0x761a, 0x7669, 0x76b7,
    0x7703, 0x774e, 0x7798, 0x77e1, 0x7828, 0x786f, 0x78b4, 0x78f8, 0x793b, 0x797c,
    0x79bc, 0x79fb, 0x7a39, 0x7a76, 0x7ab1, 0x7aeb, 0x7b24, 0x7b5b, 0x7b92, 0x7bc7,
    0x7bfb, 0x7c2d, 0x7c5e, 0x7c8e, 0x7cbd, 0x7ceb, 0x7d17, 0x7d42, 0x7d6c, 0x7d94,
    0x7dbc, 0x7de2, 0x7e06, 0x7e2a, 0x7e4c, 0x7e6d, 0x7e8c, 0x7eaa, 0x7ec7, 0x7ee3,
    0x7efe, 0x7f17, 0x7f2f, 0x7f45, 0x7f5b, 0x7f6f, 0x7f81, 0x7f93, 0x7fa3, 0x7fb2,
    0x7fbf, 0x7fcc, 0x7fd7, 0x7fe0, 0x7fe9, 0x7ff0, 0x7ff6, 0x7ffa, 0x7ffd, 0x7fff,
    0x7fff, 0x7fff, 0x7ffd, 0x7ffa, 0x7ff6, 0x7ff0, 0x7fe9, 0x7fe0, 0x7fd7, 0x7fcc,
    0x7fbf, 0x7fb2, 0x7fa3, 0x7f93, 0x7f81, 0x7f6f, 0x7f5b, 0x7f45, 0x7f2f, 0x7f17,
    0x7efe, 0x7ee3, 0x7ec7, 0x7eaa, 0x7e8c, 0x7e6d, 0x7e4c, 0x7e2a, 0x7e06, 0x7de2,
    0x7dbc, 0x7d94, 0x7d6c, 0x7d42, 0x7d17, 0x7ceb, 0x7cbd, 0x7c8e, 0x7c5e, 0x7c2d,
    0x7bfb, 0x7bc7, 0x7b92, 0x7b5b, 0x7b24, 0x7aeb, 0x7ab1, 0x7a76, 0x7a39, 0x79fb,
    0x79bc, 0x797c, 0x793b, 0x78f8, 0x78b4, 0x786f, 0x7828, 0x77e1, 0x7798, 0x774e,
    0x7703, 0x76b7, 0x7669, 0x761a, 0x75ca, 0x7579, 0x7527, 0x74d3, 0x747e, 0x7428,
    0x73d1, 0x7379, 0x7320, 0x72c5, 0x7269, 0x720d, 0x71ae, 0x714f, 0x70ef, 0x708d,
    0x702b, 0x6fc7, 0x6f62, 0x6efc, 0x6e95, 0x6e2d, 0x6dc3, 0x6d59, 0x6ced, 0x6c81,
    0x6c13, 0x6ba4, 0x6b34, 0x6ac3, 0x6a51, 0x69de, 0x696a, 0x68f4, 0x687e, 0x6806,
    0x678e, 0x6714, 0x669a, 0x661e, 0x65a1, 0x6524, 0x64a5, 0x6425, 0x63a5, 0x6323,
    0x62a0, 0x621c, 0x6198, 0x6112, 0x608b, 0x6004, 0x5f7b, 0x5ef1, 0x5e67, 0x5ddb,
    0x5d4f, 0x5cc1, 0x5c33, 0x5ba4, 0x5b14, 0x5a82, 0x59f0, 0x595d, 0x58ca, 0x5835,
    0x579f, 0x5709, 0x5671, 0x55d9, 0x5540, 0x54a6, 0x540b, 0x536f, 0x52d3, 0x5235,
    0x5197, 0x50f8, 0x5058, 0x4fb8, 0x4f16, 0x4e74, 0x4dd1, 0x4d2d, 0x4c88, 0x4be3,
    0x4b3d, 0x4a96, 0x49ee, 0x4945, 0x489c, 0x47f2, 0x4748, 0x469c, 0x45f0, 0x4543,
    0x4496, 0x43e8, 0x4339, 0x4289, 0x41d9, 0x4128, 0x4077, 0x3fc5, 0x3f12, 0x3e5e,
    0x3daa, 0x3cf5, 0x3c40, 0x3b8a, 0x3ad4, 0x3a1c, 0x3965, 0x38ac, 0x37f3, 0x373a,
    0x3680, 0x35c5, 0x350a, 0x344f, 0x3392, 0x32d6, 0x3219, 0x315b, 0x309d, 0x2fde,
    0x2f1f, 0x2e5f, 0x2d9f, 0x2cde, 0x2c1d, 0x2b5c, 0x2a9a, 0x29d7, 0x2915, 0x2851,
    0x278e, 0x26ca, 0x2605, 0x2541, 0x247c, 0x23b6, 0x22f0, 0x222a, 0x2163, 0x209c,
    0x1fd5, 0x1f0d, 0x1e46, 0x1d7d, 0x1cb5, 0x1bec, 0x1b23, 0x1a5a, 0x1990, 0x18c6,
    0x17fc, 0x1732, 0x1667, 0x159c, 0x14d1, 0x1406, 0x133b, 0x126f, 0x11a3, 0x10d7,
    0x100b, 0x0f3f, 0x0e72, 0x0da5, 0x0cd9, 0x0c0c, 0x0b3f, 0x0a72, 0x09a4, 0x08d7,
    0x080a, 0x073c, 0x066e, 0x05a1, 0x04d3, 0x0405, 0x0337, 0x026a, 0x019c, 0x00ce,
    0x0000, 0xff32, 0xfe64, 0xfd96, 0xfcc9, 0xfbfb, 0xfb2d, 0xfa5f, 0xf992, 0xf8c4,
    0xf7f6, 0xf729, 0xf65c, 0xf58e, 0xf4c1, 0xf3f4, 0xf327, 0xf25b, 0xf18e, 0xf0c1,
    0xeff5, 0xef29, 0xee5d, 0xed91, 0xecc5, 0xebfa, 0xeb2f, 0xea64, 0xe999, 0xe8ce,
    0xe804, 0xe73a, 0xe670, 0xe5a6, 0xe4dd, 0xe414, 0xe34b, 0xe283, 0xe1ba, 0xe0f3,
    0xe02b, 0xdf64, 0xde9d, 0xddd6, 0xdd10, 0xdc4a, 0xdb84, 0xdabf, 0xd9fb, 0xd936,
    0xd872, 0xd7af, 0xd6eb, 0xd629, 0xd566, 0xd4a4, 0xd3e3, 0xd322, 0xd261, 0xd1a1,
    0xd0e1, 0xd022, 0xcf63, 0xcea5, 0xcde7, 0xcd2a, 0xcc6e, 0xcbb1, 0xcaf6, 0xca3b,
    0xc980, 0xc8c6, 0xc80d, 0xc754, 0xc69b, 0xc5e4, 0xc52c, 0xc476, 0xc3c0, 0xc30b,
    0xc256, 0xc1a2, 0xc0ee, 0xc03b, 0xbf89, 0xbed8, 0xbe27, 0xbd77, 0xbcc7, 0xbc18,
    0xbb6a, 0xbabd, 0xba10, 0xb964, 0xb8b8, 0xb80e, 0xb764, 0xb6bb, 0xb612, 0xb56a,
    0xb4c3, 0xb41d, 0xb378, 0xb2d3, 0xb22f, 0xb18c, 0xb0ea, 0xb048, 0xafa8, 0xaf08,
    0xae69, 0xadcb, 0xad2d, 0xac91, 0xabf5, 0xab5a, 0xaac0, 0xaa27, 0xa98f, 0xa8f7,
    0xa861, 0xa7cb, 0xa736, 0xa6a3, 0xa610, 0xa57e, 0xa4ec, 0xa45c, 0xa3cd, 0xa33f,
    0xa2b1, 0xa225, 0xa199, 0xa10f, 0xa085, 0x9ffc, 0x9f75, 0x9eee, 0x9e68, 0x9de4,
    0x9d60, 0x9cdd, 0x9c5b, 0x9bdb, 0x9b5b, 0x9adc, 0x9a5f, 0x99e2, 0x9966, 0x98ec,
    0x9872, 0x97fa, 0x9782, 0x970c, 0x9696, 0x9622, 0x95af, 0x953d, 0x94cc, 0x945c,
    0x93ed, 0x937f, 0x9313, 0x92a7, 0x923d, 0x91d3, 0x916b, 0x9104, 0x909e, 0x9039,
    0x8fd5, 0x8f73, 0x8f11, 0x8eb1, 0x8e52, 0x8df3, 0x8d97, 0x8d3b, 0x8ce0, 0x8c87,
    0x8c2f, 0x8bd8, 0x8b82, 0x8b2d, 0x8ad9, 0x8a87, 0x8a36, 0x89e6, 0x8997, 0x8949,
    0x88fd, 0x88b2, 0x8868, 0x881f, 0x87d8, 0x8791, 0x874c, 0x8708, 0x86c5, 0x8684,
    0x8644, 0x8605, 0x85c7, 0x858a, 0x854f, 0x8515, 0x84dc, 0x84a5, 0x846e, 0x8439,
    0x8405, 0x83d3, 0x83a2, 0x8372, 0x8343, 0x8315, 0x82e9, 0x82be, 0x8294, 0x826c,
    0x8244, 0x821e, 0x81fa, 0x81d6, 0x81b4, 0x8193, 0x8174, 0x8156, 0x8139, 0x811d,
    0x8102, 0x80e9, 0x80d1, 0x80bb, 0x80a5, 0x8091, 0x807f, 0x806d, 0x805d, 0x804e,
    0x8041, 0x8034, 0x8029, 0x8020, 0x8017, 0x8010, 0x800a, 0x8006, 0x8003, 0x8001,
    0x8000, 0x8001, 0x8003, 0x8006, 0x800a, 0x8010, 0x8017, 0x8020, 0x8029, 0x8034,
    0x8041, 0x804e, 0x805d, 0x806d, 0x807f, 0x8091, 0x80a5, 0x80bb, 0x80d1, 0x80e9,
    0x8102, 0x811d, 0x8139, 0x8156, 0x8174, 0x8193, 0x81b4, 0x81d6, 0x81fa, 0x821e,
    0x8244, 0x826c, 0x8294, 0x82be, 0x82e9, 0x8315, 0x8343, 0x8372, 0x83a2, 0x83d3,
    0x8405, 0x8439, 0x846e, 0x84a5, 0x84dc, 0x8515, 0x854f, 0x858a, 0x85c7, 0x8605,
    0x8644, 0x8684, 0x86c5, 0x8708, 0x874c, 0x8791, 0x87d8, 0x881f, 0x8868, 0x88b2,
    0x88fd, 0x8949, 0x8997, 0x89e6, 0x8a36, 0x8a87, 0x8ad9, 0x8b2d, 0x8b82, 0x8bd8,
    0x8c2f, 0x8c87, 0x8ce0, 0x8d3b, 0x8d97, 0x8df3, 0x8e52, 0x8eb1, 0x8f11, 0x8f73,
    0x8fd5, 0x9039, 0x909e, 0x9104, 0x916b, 0x91d3, 0x923d, 0x92a7, 0x9313, 0x937f,
    0x93ed, 0x945c, 0x94cc, 0x953d, 0x95af, 0x9622, 0x9696, 0x970c, 0x9782, 0x97fa,
    0x9872, 0x98ec, 0x9966, 0x99e2, 0x9a5f, 0x9adc, 0x9b5b, 0x9bdb, 0x9c5b, 0x9cdd,
    0x9d60, 0x9de4, 0x9e68, 0x9eee, 0x9f75, 0x9ffc, 0xa085, 0xa10f, 0xa199, 0xa225,
    0xa2b1, 0xa33f, 0xa3cd, 0xa45c, 0xa4ec, 0xa57e, 0xa610, 0xa6a3, 0xa736, 0xa7cb,
    0xa861, 0xa8f7, 0xa98f, 0xaa27, 0xaac0, 0xab5a, 0xabf5, 0xac91, 0xad2d, 0xadcb,
    0xae69, 0xaf08, 0xafa8, 0xb048, 0xb0ea, 0xb18c, 0xb22f, 0xb2d3, 0xb378, 0xb41d,
    0xb4c3, 0xb56a, 0xb612, 0xb6bb, 0xb764, 0xb80e, 0xb8b8, 0xb964, 0xba10, 0xbabd,
    0xbb6a, 0xbc18, 0xbcc7, 0xbd77, 0xbe27, 0xbed8, 0xbf89, 0xc03b, 0xc0ee, 0xc1a2,
    0xc256, 0xc30b, 0xc3c0, 0xc476, 0xc52c, 0xc5e4, 0xc69b, 0xc754, 0xc80d, 0xc8c6,
    0xc980, 0xca3b, 0xcaf6, 0xcbb1, 0xcc6e, 0xcd2a, 0xcde7, 0xcea5, 0xcf63, 0xd022,
    0xd0e1, 0xd1a1, 0xd261, 0xd322, 0xd3e3, 0xd4a4, 0xd566, 0xd629, 0xd6eb, 0xd7af,
    0xd872, 0xd936, 0xd9fb, 0xdabf, 0xdb84, 0xdc4a, 0xdd10, 0xddd6, 0xde9d, 0xdf64,
    0xe02b, 0xe0f3, 0xe1ba, 0xe283, 0xe34b, 0xe414, 0xe4dd, 0xe5a6, 0xe670, 0xe73a,
    0xe804, 0xe8ce, 0xe999, 0xea64, 0xeb2f, 0xebfa, 0xecc5, 0xed91, 0xee5d, 0xef29,
    0xeff5, 0xf0c1, 0xf18e, 0xf25b, 0xf327, 0xf3f4, 0xf4c1, 0xf58e, 0xf65c, 0xf729,
    0xf7f6, 0xf8c4, 0xf992, 0xfa5f, 0xfb2d, 0xfbfb, 0xfcc9, 0xfd96, 0xfe64, 0xff32};

const int16_t phasearray[360]={
  0, 
  2, 5, 8, 11, 13, 16, 19, 22, 25, 27, 30, 33, 36, 38, 41, 44, 
  47, 50, 52, 55, 58, 61, 63, 66, 69, 72, 75, 77, 80, 83, 86, 88, 
  91, 94, 97, 100, 102, 105, 108, 111, 113, 116, 119, 122, 125, 127, 130, 133, 
  136, 138, 141, 144, 147, 150, 152, 155, 158, 161, 163, 166, 169, 172, 175, 177, 
  180, 183, 186, 188, 191, 194, 197, 200, 202, 205, 208, 211, 213, 216, 219, 222, 
  225, 227, 230, 233, 236, 238, 241, 244, 247, 250, 252, 255, 258, 261, 263, 266, 
  269, 272, 275, 277, 280, 283, 286, 288, 291, 294, 297, 300, 302, 305, 308, 311, 
  313, 316, 319, 322, 325, 327, 330, 333, 336, 338, 341, 344, 347, 350, 352, 355, 
  358, 361, 363, 366, 369, 372, 375, 377, 380, 383, 386, 388, 391, 394, 397, 400, 
  402, 405, 408, 411, 413, 416, 419, 422, 425, 427, 430, 433, 436, 438, 441, 444, 
  447, 450, 452, 455, 458, 461, 463, 466, 469, 472, 475, 477, 480, 483, 486, 488, 
  491, 494, 497, 500, 502, 505, 508, 511, 513, 516, 519, 522, 525, 527, 530, 533, 
  536, 538, 541, 544, 547, 550, 552, 555, 558, 561, 563, 566, 569, 572, 575, 577, 
  580, 583, 586, 588, 591, 594, 597, 600, 602, 605, 608, 611, 613, 616, 619, 622, 
  625, 627, 630, 633, 636, 638, 641, 644, 647, 650, 652, 655, 658, 661, 663, 666, 
  669, 672, 675, 677, 680, 683, 686, 688, 691, 694, 697, 700, 702, 705, 708, 711, 
  713, 716, 719, 722, 725, 727, 730, 733, 736, 738, 741, 744, 747, 750, 752, 755, 
  758, 761, 763, 766, 769, 772, 775, 777, 780, 783, 786, 788, 791, 794, 797, 800, 
  802, 805, 808, 811, 813, 816, 819, 822, 825, 827, 830, 833, 836, 838, 841, 844, 
  847, 850, 852, 855, 858, 861, 863, 866, 869, 872, 875, 877, 880, 883, 886, 888, 
  891, 894, 897, 900, 902, 905, 908, 911, 913, 916, 919, 922, 925, 927, 930, 933, 
  936, 938, 941, 944, 947, 950, 952, 955, 958, 961, 963, 966, 969, 972, 975, 977, 
  980, 983, 986, 988, 991, 994, 997};
  
uint16_t lowsamp_table[160]={
  0, 
  160, 80, 53, 40, 32, 26, 22, 20, 17, 16, 14, 13, 12, 11, 10, 10, 
  9, 8, 8, 8, 7, 7, 6, 6, 6, 6, 5, 5, 5, 5, 5, 5, 
  4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 
  3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};



// freq10 is the desired freq multiplied by 10
void osc_init(char freqonly, uint8_t n, int16_t amplitude, unsigned int freq10, uint16_t phase, char envelope)
{
	  double lowfreq;
	  double grad;
	  osc[n].lowsamp=0;
	  osc[n].env=0;
	  osc[n].env_phase=0;
    // phase is a value between 0 and 359
    // sanity
    if (freq10>65535)
    	freq10=65535;
    if (freq10<0)
    	freq10=0;
    osc[n].freq10=freq10;
    
    
    if (freqonly==OSC_INIT)
    {
    	//printf("OSC_INIT\n");
      osc[n].phase=phase;
      osc[n].amplitude=amplitude;
      osc[n].idx=phasearray[phase];
    }
    else
    {
      //printf("requested oper %d: amp %d, freq10 %d, phase %d\n", n, amplitude, freq10, phase);
    }

    
    
    
    if (freq10<160)
    {
    	osc[n].lowsamp=lowsamp_table[freq10];
    	osc[n].incr=0;     	
    }
    else
    {
    	osc[n].incr=freq10/160;
    }
    
    // check if we need to set up envelope mode
    if ((envelope!=0) && (freqonly==OSC_INIT))
    {
    	  osc[n].incr=0;
    		// At low frequencies, non-sinewaves are allowed as enveloped
    		if (envelope==ENV_ADSR)
        {
    			osc[n].env=ENV_ADSR;
    			osc[n].env_phase=PHASE_ATTACK;
    			lowfreq=((double)freq10)/10;
    			// use lowsamp to just hold the overall period in hundredths of a sec
    			osc[n].lowsamp=(uint16_t)(100/lowfreq);
    			// 16000 samples per sec, or 160 samples per hundredth of a sec
  	  		// attack phase is one sixth of the overall period
  	  		grad=2.048 * 6.0 * lowfreq;
    			osc[n].m = (uint16_t)grad;
    			
        }
    } // end set up envelope mode
    
}


int16_t
oper_run(uint8_t n, int16_t amp, unsigned int freq10)
{
	float factor;
  int16_t out;
  uint16_t lowsamp;
  uint16_t idx;
  uint16_t freq;
  
  // if we are in envelope mode, we do not run the sinewave oscillator
  if (osc[n].env!=0)
  {
  	if (osc[n].env==ENV_ADSR)
  	{
  	  switch(osc[n].env_phase)
  	  {
  	  	case PHASE_ATTACK:
  	  		out=(int16_t)(osc[n].m * osc[n].incr);
  	  		osc[n].incr++;
  	  		if (osc[n].incr >= (osc[n].lowsamp * 160/6))
  	  		{
  	  		  osc[n].env_phase=PHASE_DECAY;
  	  		  // now we reduce the gradient
  	  		  osc[n].m=osc[n].m/8;
  	  		  osc[n].incr=0;
  	  	  }
  	  		break;	
  	    case PHASE_DECAY:
  	    	out=32767-((int16_t)(osc[n].m * osc[n].incr));
  	    	osc[n].incr++;
  	    	if (osc[n].incr >= (osc[n].lowsamp * 160/6))
  	  		{
  	    	  osc[n].env_phase=PHASE_SUSTAIN;
  	    	  osc[n].outbuf=out;
  	    	  osc[n].incr=0;
  	      }
  	    	break;
  	    case PHASE_SUSTAIN:
  	    	out=osc[n].outbuf;
  	    	osc[n].incr++;
  	    	if (osc[n].incr >= (osc[n].lowsamp * 160/2))
  	    	{
  	    		osc[n].env_phase=PHASE_RELEASE;
  	    		osc[n].incr=0;
  	    		osc[n].m=osc[n].outbuf/(osc[n].lowsamp * 160/8);
  	    	}
  	    	break;
  	    case PHASE_RELEASE:
  	    	out=osc[n].outbuf-(osc[n].m * osc[n].incr);
  	    	if (out<0)
  	    		out=0;
  	    	osc[n].incr++;
  	    	if (osc[n].incr >= (osc[n].lowsamp * 160/6))
  	    	{
  	    		osc[n].env_phase=PHASE_COMPLETE;
  	    		osc[n].incr=0;
  	    	}
  	    	break;
  	    case PHASE_COMPLETE:
  	    	printf("C ");
  	      // just play out silence
  	      out=0;
  	      break;
  	    default:
  	    	printf("Error - unknown env_phase!\n");
  	    	break;
  	  } // end switch
  	
    } // end ENV_ADSR
    
    // now we need to scale the output using the amplitude value
    if (amp<MAXVOL)
    {
      out=( (((uint32_t)out)*((uint32_t)amp))/MAXVOL);
    }
    
    return(out); 
  } // end of envelope mode
  
  // normal oscillator mode
  if (freq10!=osc[n].freq10)
  {
  	// the frequency has changed
  	//printf("freq change!! \n");
  	osc_init(FREQ_UPDATE, n, 0, freq10, 0, 0); // update freq
  }
  lowsamp=osc[n].lowsamp;
  idx=osc[n].idx;
  freq=osc[n].freq;
  
  out=sinearr[idx];
  //printf("osc %d, amp %d, idx %d, out %d\n", n, amp, idx, out);

  if (lowsamp==0) // the desired frequency is higher than 16Hz
  {
  	idx=idx+(osc[n].incr);
  	if (idx>=1000)
    {
      idx=idx-1000;
    }
  }
  else
  {
  	// the frequency is very low, so we need to output the same value for a while
  	osc[n].incr++;
  	if (osc[n].incr>=lowsamp)
    {
    	idx++;
    	osc[n].incr=0;
    	if (idx>=1000)
    	{
    		idx=0;
    	}
    }
  }
  // now we need to scale the output using the amplitude value
  if (amp<MAXVOL)
  {
    out=( (((uint32_t)out)*((uint32_t)amp))/MAXVOL);
  }
  
  osc[n].idx=idx;
  
  return(out);
}

void voice_run(uint16_t* buf, uint16_t iter)
{
	uint16_t* bp;
	uint16_t iloop;
	int16_t ug1, ug2, ug3, ug4, ug5, ug6;
	
	bp=buf;
	
	for (iloop=0; iloop<iter; iloop++)
	{
    // play out operator (osc) 0
    ug4=oper_run(OPER_0, osc[OPER_0].amplitude, osc[OPER_0].freq10);
    ug5=oper_run(OPER_1, osc[OPER_1].amplitude, osc[OPER_1].freq10);
    ug6=(ug5/2)+(inst.dev1/2);
    ug1=oper_run(OPER_2, ug6, inst.modfreq10);
    ug2=(inst.carrfreq)+(ug1);
    
    if (ug2<0)
    	ug2=0-ug2;
    ug3=oper_run(OPER_3, ug4, ((uint16_t)ug2)*10);
    if (inst.carrfreq==0)
    {
      ug3=0;
    }
    
    *buf++=(uint16_t)ug3;
    bp++;
  } // for iloop
}

int
main(void)
{
  FILE* fp;
  int i;
  int j;
  double duration = 1; // seconds
  int32_t FrameCount = duration * SAMPLE_RATE;
  int cycles=200;
  uint16_t nidx;
  char not_finished=1;
  int16_t m_noteframes84;
  uint16_t notefreq;
  int16_t length;

  // instrument configuration parameters
  inst.amp=32767;
  inst.dur_recip10=20;
  inst.carrfreq=900;
  inst.dev1=40;
  inst.dev2=40;
  inst.modfreq10=6000;

  // initialize the operators
  osc_init(OSC_INIT, OPER_0, inst.amp, inst.dur_recip10, 0, ENV_ADSR /*env*/);
  osc_init(OSC_INIT, OPER_1, inst.dev2, inst.dur_recip10, 0, ENV_ADSR);
  osc_init(OSC_INIT, OPER_2, 0, inst.modfreq10, 0, 0);
  osc_init(OSC_INIT, OPER_3, 0, 999, 0, 0); // dummy freq
  

  fp=fopen("out.wav", "w");
  if (fp==NULL)
  {
    perror("file open error!");
    exit(1);
  }
  
  buffer_p = allocate_PCM16_stereo_buffer(FRAME_SIZE);
  if (buffer_p==NULL)
  {
    perror("failed to allocate buffer for WAV file!");
    exit(1);
  }

  // set number of frames for one eigth of a quarter note. 
  m_noteframes84=12;  


  // work out how many cycles there will be, so we can write
  // the WAV header
  not_finished=1; nidx=0;; cycles=0;
  do
  {
    cycles=cycles+poly1periods[nidx];
    nidx++;
    if (poly1periods[nidx]>=99)
    	not_finished=0;
  } while (not_finished);
  cycles=cycles*m_noteframes84;
  // write header
  write_PCM16_stereo_header(fp, SAMPLE_RATE, (int32_t)(FRAME_SIZE*cycles));


  not_finished=1;  
  nidx=0;
  do
  {
  	// pick a note
  	if (poly1keys[nidx]<60)
  	{
  	  notefreq=keyfreq[poly1keys[nidx]+12]; // add or subtract 12 to shift an octave
    }
    else
    {
      if (poly1keys[nidx]==PAUSE)
      	notefreq=0;
    }
    
  	length=poly1periods[nidx];
  	
  	// set up the voice
  	inst.carrfreq=notefreq;
  	inst.dur_recip10=2000/(length*m_noteframes84); 
  	osc_init(OSC_INIT, OPER_0, inst.amp, inst.dur_recip10, 0, ENV_ADSR /*env*/);
    osc_init(OSC_INIT, OPER_1, inst.dev2, inst.dur_recip10, 0, ENV_ADSR);
    osc_init(OSC_INIT, OPER_2, 0, inst.modfreq10, 0, 0);
    osc_init(OSC_INIT, OPER_3, 0, 999, 0, 0); // dummy freq
      
  	for (j=0; j<m_noteframes84*length; j++)
  	{
  		voice_run(buf0, FRAME_SIZE);
  		for (i=0; i<FRAME_SIZE; i++)
      {
        buffer_p[i].left=buf0[i];
      }

      write_PCM16wav_data(fp, FRAME_SIZE, buffer_p);
    }
    nidx++;
    if (poly1keys[nidx]>=99)
    {
    	// we're done
    	not_finished=0;
    }
  
  } while(not_finished); 
  
  printf("Done.. out.wav has been written. type aplay out.wav to play it!\n");

  free(buffer_p);
  fclose(fp); 
  return(0);
}




