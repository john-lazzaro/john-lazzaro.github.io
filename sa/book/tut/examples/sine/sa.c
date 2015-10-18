
/*
#    Sfront, a SAOL to C translator    
#    This file: Included file in sfront runtime
#
# Copyright (c) 1999-2006, Regents of the University of California
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#  Redistributions of source code must retain the above copyright
#  notice, this list of conditions and the following disclaimer.
#
#  Redistributions in binary form must reproduce the above copyright
#  notice, this list of conditions and the following disclaimer in the
#  documentation and/or other materials provided with the distribution.
#
#  Neither the name of the University of California, Berkeley nor the
#  names of its contributors may be used to endorse or promote products
#  derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#    Maintainer: John Lazzaro, lazzaro@cs.berkeley.edu
*/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <signal.h>

/********************************/
/* readabiliy-improving defines */
/********************************/

#define NV(x)   nstate->v[x].f
#define NVI(x)  nstate->v[x].i
#define NVUI(x) nstate->v[x].ui
#define NVU(x)  nstate->v[x]
#define NT(x)   nstate->t[x]
#define NS(x)   nstate->x
#define NSP     nstate
#define NP(x)   nstate->v[x].f
#define NPI(x)  nstate->v[x].i
#define NPUI(x) nstate->v[x].ui
#define NG(x)   global[x].f
#define NGI(x)  global[x].i
#define NGUI(x) global[x].ui
#define NGU(x)  global[x]

#define TB(x)   bus[x]
#define STB(x)  sbus[x]
#define ROUND(x) ( ((x) > 0.0F) ? ((int) ((x) + 0.5F)) :  ((int) ((x) - 0.5F)))
#define POS(x)   (((x) > 0.0F) ? x : 0.0F)
#define RMULT ((float)(1.0F/(RAND_MAX + 1.0F)))

#define NOTUSEDYET 0
#define TOBEPLAYED 1
#define PAUSED     2
#define PLAYING    3
#define ALLDONE    4

#define NOTLAUNCHED 0
#define LAUNCHED 1

#define ASYS_DONE        0
#define ASYS_EXIT        1
#define ASYS_ERROR       2

#define IPASS 1
#define KPASS 2
#define APASS 3

#define IOERROR_RETRY 256 

/************************************/
/* externs for system functions     */
/************************************/

extern void epr(int, char *, char *, char *);
extern size_t rread(void * ptr, size_t len, size_t nmemb, FILE * stream);
extern size_t rwrite(void * ptr, size_t len, size_t nmemb, FILE * stream);

/************************************/
/*  union for a data stack element  */
/************************************/

typedef union {

float f;
long  i;
unsigned long ui;

} dstack;


/************************************/
/* ntables: table entries for notes */
/************************************/

typedef struct tableinfo {

int    len;                /* length of table */
float  lenf;               /* length of table, as a float */

int    start;              /* loop start position */
int    end;                /* loop end position */
float  sr;                 /* table sampling rate  */
float  base;               /* table base frequency */

                           /* precomputed constants       */
int tend;                  /* len -1 if end==0            */
float oconst;              /* len*ATIME                   */

unsigned long dint;        /* doscil: 64-bit phase incr   */
unsigned long dfrac;
                           /* doscil: sinc interpolation        */
unsigned long sfui;        /* scale_factor as unsigned long     */
float sffl;                /* scale_factor as a float           */
unsigned long dsincr;      /* sinc pointer increment (d=doscil) */

float  *t;                 /* pointer to table entries    */
float stamp;               /* timestamp on table contents */
char llmem;                /* 1 if *t was malloced        */
} tableinfo; 

/********************/
/*  control lines   */
/********************/

typedef struct scontrol_lines {

float t;                  /* trigger time */
int label;                /* index into label array */
int siptr;                /* score instr line to control */
struct instr_line *iline; /* pointer to score line */
int imptr;                /* position of variable in v[] */
float imval;              /* value to import into v[] */

} scontrol_lines;

/********************/
/*   tempo lines    */
/********************/

typedef struct stempo_lines {

  float t;          /* trigger time */
  float newtempo;   /* new tempo */ 

} stempo_lines;

/********************/
/*   table lines    */
/********************/

typedef struct stable_lines {

  float t;          /* trigger time */
  int gindex;       /* global table to target */
  int size;         /* size of data */
  void (*tmake) (); /* function   */
  void * data;      /* data block */

} stable_lines;

/********************/
/* system variables */
/********************/

/* audio and control rates */

float globaltune = 440.0F;
float invglobaltune = 2.272727e-03F;
float scorebeats = 0.0F;              /* current score beat */
float absolutetime = 0.0F;            /* current absolute time */
int kbase = 1;                        /* kcycle of last tempo change */
float scorebase = 0.0F;               /* scorebeat of last tempo change */

/* counters & bounds acycles and kcycles */

int endkcycle;
int kcycleidx = 1;
int acycleidx = 0;
int pass = IPASS;
int beginflag;
sig_atomic_t graceful_exit;

struct instr_line * sysidx;

int busidx;        /* counter for buses                   */
int nextstate = 0; /* counter for active instrument state */
int oldstate;      /* detects loops in nextstate updates  */
int tstate;        /* flag for turnoff state machine      */
float cpuload;     /* current cpu-time value              */

int asys_argc;
char ** asys_argv;

int csys_argc;
char ** csys_argv;


int csys_sfront_argc = 7;

char * csys_sfront_argv[7] = {
"sfront",
"-aout",
"output.wav",
"-orc",
"sine.saol",
"-sco",
"sine.sasl"};


#define APPNAME "sfront"
#define APPVERSION "--IDSTRING--"
#define NSYS_NET 0
#define INTERP_LINEAR 0
#define INTERP_SINC 1
#define INTERP_TYPE INTERP_LINEAR
#define ARATE 32000.0F
#define ATIME 3.125000e-05F
#define KRATE 100.0F
#define KTIME 1.000000e-02F
#define KMTIME 1.000000e+01F
#define KUTIME 10000L
#define ACYCLE 320L
float tempo = 60.0F;
float scoremult = 1.000000e-02F;

#define ASYS_RENDER   0
#define ASYS_PLAYBACK 1
#define ASYS_TIMESYNC 2

#define ASYS_SHORT   0
#define ASYS_FLOAT   1

/* audio i/o */

#define ASYS_OCHAN 1L
#define ASYS_OTYPENAME  ASYS_FLOAT
#define ASYS_OTYPE  float
long asys_osize = 0;
long obusidx = 0;

ASYS_OTYPE * asys_obuf = NULL;

#define ASYS_USERLATENCY  0
#define ASYS_LOWLATENCY   0
#define ASYS_HIGHLATENCY  1
#define ASYS_LATENCYTYPE  ASYS_HIGHLATENCY
#define ASYS_LATENCY 0.300000F
#define ASYS_TIMEOPTION ASYS_RENDER

/* AIF or WAV output file wordsize */

#define ASYS_OUTFILE_WORDSIZE_8BIT  0
#define ASYS_OUTFILE_WORDSIZE_16BIT  1
#define ASYS_OUTFILE_WORDSIZE_24BIT  2
#define ASYS_OUTFILE_WORDSIZE  1

#define MAXPFIELDS 1

struct instr_line { 

float starttime;  /* score start time of note */
float endtime;    /* score end time of note */
float startabs;   /* absolute start time of note */
float endabs;     /* absolute end time of note */
float abstime;    /* absolute time extension */
float time;       /* time of note start (absolute) */
float itime;      /* elapsed time (absolute) */
float sdur;       /* duration of note in score time*/

int kbirth;       /* kcycleidx for note launch */
int released;     /* flag for turnoff*/
int turnoff;      /* flag for turnoff */
int noteon;       /* NOTYETUSED,TOBEPLAYED,PAUSED,PLAYING,ALLDONE */
int notestate;    /* index into state array */
int launch;       /* only for dynamic instruments */
int numchan;      /* only for MIDI notes */
int preset;       /* only for MIDI notes */
int notenum;      /* only for MIDI notes */
int label;        /* SASL label index + 1, or 0 (no label) */

                  /* for static MIDI, all-sounds noteoff */

float p[MAXPFIELDS];          /* parameters */

struct ninstr_types * nstate; /* pointer into state array */

};

#define BUS_output_bus 0
#define ENDBUS_output_bus 1
#define ENDBUS 1
float bus[ENDBUS];

float fakeMIDIctrl[256];

#define GBL_STARTVAR 0
#define GBL_ENDVAR 0
/* global variables */

dstack global[GBL_ENDVAR+1];

#define GBL_ENDTBL 0
struct tableinfo gtables[GBL_ENDTBL+1];

#define tone_a 0
#define tone_x 1
#define tone_y 2
#define tone_init 3
#define tone_ENDVAR 4

#define tone_ENDTBL 0

extern void sigint_handler(int);

void tone_ipass(struct ninstr_types *);
void tone_kpass(struct ninstr_types *);
void tone_apass(struct ninstr_types *);




#define MAXENDTIME 1E+37

float endtime = 4.50F;
struct instr_line s_tone[1] = {
{ 0.25F, MAXENDTIME, MAXENDTIME, MAXENDTIME,  0.0F, 0.0F, 0.0F,  4.0F, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, { 0.0F }, NULL }};

#define MAXSTATE 2


#define MAXVARSTATE 4
#define MAXTABLESTATE 1

/* ninstr: used for score, effects, */
/* and dynamic instruments          */

struct ninstr_types {

struct instr_line * iline; /* pointer to score line */
dstack v[MAXVARSTATE];     /* parameters & variables*/
struct tableinfo t[MAXTABLESTATE]; /* tables */

} ninstr[MAXSTATE];

#define ASYS_OUTDRIVER_WAV
#define ASYS_HASOUTPUT


/*
#    Sfront, a SAOL to C translator    
#    This file: WAV audio driver for sfront
#
# Copyright (c) 1999-2006, Regents of the University of California
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#  Redistributions of source code must retain the above copyright
#  notice, this list of conditions and the following disclaimer.
#
#  Redistributions in binary form must reproduce the above copyright
#  notice, this list of conditions and the following disclaimer in the
#  documentation and/or other materials provided with the distribution.
#
#  Neither the name of the University of California, Berkeley nor the
#  names of its contributors may be used to endorse or promote products
#  derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#    Maintainer: John Lazzaro, lazzaro@cs.berkeley.edu
*/


/****************************************************************/
/****************************************************************/
/*             wav file audio driver for sfront                 */ 
/****************************************************************/
        
#include <stdio.h>
#include <string.h>

#if defined(ASYS_HASOUTPUT)

/* default name for output audio file */
#define ASYSO_DEFAULTNAME "output.wav"

/* global variables, must start with asyso_ */

FILE * asyso_fd;     /* output file pointer */
char * asyso_name;   /* name of file  */        
long asyso_srate;    /* sampling rate */
long asyso_channels; /* number of channels */
long asyso_size;     /* number of float samples _buf */
long asyso_nsamp;    /* total number of samples written */
long asyso_bps;      /* number of bytes per sample, 1-3 */
long asyso_doswap;   /* needs byteswap on write */
float * asyso_buf;   /* output floats from sfront */ 
unsigned char * asyso_cbuf;  /* output chars to file */
#endif

#if defined(ASYS_HASINPUT)

/* default name for input audio file */

#define ASYSI_DEFAULTNAME "input.wav"

/* only used for asysi_soundtypecheck */

#define ASYSI_MATCH  0
#define ASYSI_EOF 1
#define ASYSI_NOMATCH 2

/* global variables, must start with asysi_ */

FILE * asysi_fd;     /* input file pointer */
char * asysi_name;   /* name of file  */        
long asysi_srate;    /* sampling rate */
long asysi_channels; /* number of channels */
long asysi_bytes;     /* number of bytes in a buffer */
long asysi_nsamp;    /* total number of samples read */
long asysi_bps;      /* number of bytes per sample, 1-3 */
long asysi_doswap;   /* needs byteswap on read */
unsigned char * asysi_cbuf;  /* buffer of WAV file bytes */
float * asysi_buf;   /* float buffer for sfront */ 

#endif

#if defined(ASYS_HASOUTPUT)

/*********************************************************/
/*        writes next block of WAV/AIFF bytes            */
/*********************************************************/

int asyso_putbytes(unsigned char * c, int numbytes)

{
  if (rwrite(c, sizeof(char), numbytes, asyso_fd) != numbytes)
    return ASYS_ERROR;
  return ASYS_DONE;
}

/*********************************************************/
/*        writes unsigned long to a WAV files            */
/*********************************************************/

int asyso_putlong(unsigned long val, int numbytes)

{
  unsigned char c[4];

  if (numbytes > 4)
    return ASYS_ERROR;
  switch (numbytes) {
  case 4:
    c[0] = (unsigned char) (val&0x000000FF);
    c[1] = (unsigned char)((val >> 8)&0x000000FF);
    c[2] = (unsigned char)((val >> 16)&0x000000FF);
    c[3] = (unsigned char)((val >> 24)&0x000000FF);
    return asyso_putbytes(c, 4);
  case 3:
    c[0] = (unsigned char) (val&0x000000FF);
    c[1] = (unsigned char)((val >> 8)&0x000000FF);
    c[2] = (unsigned char)((val >> 16)&0x000000FF);
    return asyso_putbytes(c, 3);
  case 2:
    c[0] = (unsigned char) (val&0x000000FF);
    c[1] = (unsigned char)((val >> 8)&0x000000FF);
    return asyso_putbytes(c, 2);
  case 1:
    c[0] = (unsigned char) (val&0x000000FF);
    return asyso_putbytes(c,1);
  default:
    return ASYS_ERROR;
  }

}

/****************************************************************/
/*        core routine for audio output setup                   */
/****************************************************************/

int asyso_setup(long srate, long ochannels, long osize, char * name)


{
  short swaptest = 0x0100;
  char * val;

  asyso_doswap = *((char *)&swaptest);
  if (name == NULL)
    val = ASYSO_DEFAULTNAME;
  else
    val = name;

  switch (ASYS_OUTFILE_WORDSIZE) {
  case ASYS_OUTFILE_WORDSIZE_8BIT: 
    asyso_bps = 1;
    break;
  case ASYS_OUTFILE_WORDSIZE_16BIT:
    asyso_bps = 3;
    break;
  case ASYS_OUTFILE_WORDSIZE_24BIT:
    asyso_bps = 3;
    break;
  }

  asyso_name = strcpy((char *) calloc((strlen(val)+1),sizeof(char)), val);
  asyso_fd = fopen(asyso_name,"wb");
  if (asyso_fd == NULL)
    return ASYS_ERROR;

  /* preamble for wav file */

  asyso_putbytes((unsigned char *) "RIFF",4);
  asyso_putlong(0,4);       /* patched later */
  asyso_putbytes((unsigned char *) "WAVEfmt ",8);
  asyso_putlong(16,4);
  asyso_putlong(1,2);                          /* PCM  */
  asyso_putlong(ochannels,2);                  /* number of channels */
  asyso_putlong(srate,4);                      /* srate */
  asyso_putlong(srate*ochannels*asyso_bps, 4); /* bytes/sec */
  asyso_putlong(ochannels*asyso_bps, 2);       /* block align */
  asyso_putlong(8*asyso_bps, 2);               /* bits per sample */
  asyso_putbytes((unsigned char *) "data",4);
  asyso_putlong(0,4);                          /* patched later */

  asyso_srate = srate;
  asyso_channels = ochannels;
  asyso_size = osize;
  asyso_nsamp = 0;
  asyso_cbuf = (unsigned char *) malloc(osize*asyso_bps);

  if (asyso_cbuf == NULL)
    {
      fprintf(stderr, "Can't allocate WAV byte output buffer (%s).\n",
	      strerror(errno));
      return ASYS_ERROR;
    }

  asyso_buf = (float *)calloc(osize, sizeof(float));

  if (asyso_buf == NULL)
    {
      fprintf(stderr, "Can't allocate WAV float output buffer (%s).\n",
	      strerror(errno));
      return ASYS_ERROR;
    }

  return ASYS_DONE;
}

#endif

#if defined(ASYS_HASINPUT)

/*********************************************************/
/*            gets next block of WAV bytes               */
/*********************************************************/

int asysi_getbytes(unsigned char * c, int numbytes)

{
  if ((int)rread(c, sizeof(char), numbytes, asysi_fd) != numbytes)
    return ASYS_ERROR;
  return ASYS_DONE;
}

/*********************************************************/
/*        flushes next block of WAV bytes                */
/*********************************************************/

int asysi_flushbytes(int numbytes)

{
  unsigned char c;

  while (numbytes > 0)
    {
      if (rread(&c, sizeof(char), 1, asysi_fd) != 1)
	return ASYS_ERROR;
      numbytes--;
    }
  return ASYS_DONE;

}

/*********************************************************/
/*     converts byte stream to an unsigned long          */
/*********************************************************/

long asysi_getlong(int numbytes, unsigned long * ret)

{
  unsigned char c[4];

  if (numbytes > 4)
    return ASYS_ERROR;
  if (ASYS_DONE != asysi_getbytes(&c[0],numbytes))
    return ASYS_ERROR;
  switch (numbytes) {
  case 4:
    *ret  =  (unsigned long)c[0];
    *ret |=  (unsigned long)c[1] << 8;
    *ret |=  (unsigned long)c[2] << 16;
    *ret |=  (unsigned long)c[3] << 24;
    return ASYS_DONE;
  case 3:
    *ret  =  (unsigned long)c[0];
    *ret |=  (unsigned long)c[1] << 8;
    *ret |=  (unsigned long)c[2] << 16;
    return ASYS_DONE;
  case 2:
    *ret  =  (unsigned long)c[0];
    *ret |=  (unsigned long)c[1] << 8;
    return ASYS_DONE;
  case 1:
    *ret = (unsigned long)c[0];
    return ASYS_DONE;
  default:
    return ASYS_ERROR;
  }

}
  
/***********************************************************/
/*  checks byte stream for AIFF/WAV cookie --              */
/***********************************************************/

int asysi_soundtypecheck(char * d)

{
  char c[4];

  if (rread(c, sizeof(char), 4, asysi_fd) != 4)
    return ASYSI_EOF;
  if (strncmp(c,d,4))
    return ASYSI_NOMATCH;
  return ASYSI_MATCH;
}
  
/****************************************************************/
/*        core routine for audio input setup                   */
/****************************************************************/

int asysi_setup(long srate, long ichannels, long isize, char * name)


{
  short swaptest = 0x0100;
  unsigned long i, cookie;
  long len;
  char * val;

  asysi_doswap = *((char *)&swaptest);

  if (name == NULL)
    val = ASYSI_DEFAULTNAME;
  else
    val = name;
  asysi_name = strcpy((char *) calloc((strlen(val)+1),sizeof(char)), val);
  asysi_fd = fopen(asysi_name,"rb");
  if (asysi_fd == NULL)
    return ASYS_ERROR;

  if (asysi_soundtypecheck("RIFF")!= ASYSI_MATCH)
    return ASYS_ERROR;
  if (asysi_flushbytes(4)!= ASYS_DONE)
    return ASYS_ERROR;
  if (asysi_soundtypecheck("WAVE")!= ASYSI_MATCH)
    return ASYS_ERROR;
  while ((cookie = asysi_soundtypecheck("fmt "))!=ASYSI_MATCH)
    {
      if (cookie == ASYSI_EOF)
	return ASYS_ERROR;
      if (asysi_getlong(4, &i) != ASYS_DONE)
	return ASYS_ERROR;
      if (asysi_flushbytes(i + (i % 2))!= ASYS_DONE)
	return ASYS_ERROR;
    }
  if (asysi_getlong(4, &i) != ASYS_DONE)
    return ASYS_ERROR;
  len = i;
  if ((len -= 16) < 0)
    return ASYS_ERROR;
  if (asysi_getlong(2, &i) != ASYS_DONE)
    return ASYS_ERROR;
  if (i != 1)
    {
      fprintf(stderr,"Error: Can only handle PCM WAV files\n");
      return ASYS_ERROR;
    }
  if (asysi_getlong(2, &i) != ASYS_DONE)
    return ASYS_ERROR;
  if (i != ichannels)
    {
      fprintf(stderr,"Error: Inchannels doesn't match WAV file\n");
      return ASYS_ERROR;
    }
  if (asysi_getlong(4, &i) != ASYS_DONE)
    return ASYS_ERROR;
  if (srate != i)
    fprintf(stderr,"Warning: SAOL srate %i mismatches WAV file srate %i\n",
	    srate,i);
  asysi_flushbytes(6);
  if (asysi_getlong(2, &i) != ASYS_DONE)
    return ASYS_ERROR;
  if ((i < 8) || (i > 24))
    {
      fprintf(stderr,"Error: Can't handle %i bit data\n",i);
      return ASYS_ERROR;
    }
  asysi_bps = i/8;
  asysi_flushbytes(len + (len % 2));

  while ((cookie = asysi_soundtypecheck("data"))!=ASYSI_MATCH)
    {
      if (cookie == ASYSI_EOF)
	return ASYS_ERROR;
      if (asysi_getlong(4, &i) != ASYS_DONE)
	return ASYS_ERROR;
      if (asysi_flushbytes(i + (i % 2))!= ASYS_DONE)
	return ASYS_ERROR;
    }
  if (asysi_getlong(4, &i) != ASYS_DONE)
    return ASYS_ERROR;

  asysi_nsamp = i/asysi_bps;
  asysi_srate = srate;
  asysi_channels = ichannels;
  asysi_bytes = isize*asysi_bps;
  asysi_cbuf = (unsigned char *) malloc(asysi_bytes);

  if (asysi_cbuf == NULL)
    {
      fprintf(stderr, "Can't allocate WAV input byte buffer (%s).\n",
	      strerror(errno));
      return ASYS_ERROR;
    }

  asysi_buf = (float *) malloc(sizeof(float)*isize);

  if (asysi_buf == NULL)
    {
      fprintf(stderr, "Can't allocate WAV input float buffer (%s).\n",
	      strerror(errno));
      return ASYS_ERROR;
    }

  return ASYS_DONE;
}

#endif

#if (defined(ASYS_HASOUTPUT) && !defined(ASYS_HASINPUT))

/****************************************************************/
/*        sets up audio output for a given srate/channels       */
/****************************************************************/

int asys_osetup(long srate, long ochannels, long osample, 
		char * oname, long toption)

{
  return asyso_setup(srate, ochannels, ASYS_OCHAN*ACYCLE, oname);
}

#endif


#if (!defined(ASYS_HASOUTPUT) && defined(ASYS_HASINPUT))

/****************************************************************/
/*        sets up audio input for a given srate/channels       */
/****************************************************************/

int asys_isetup(long srate, long ichannels, long isample, 
		char * iname, long toption)

{
  return asysi_setup(srate, ichannels, ASYS_ICHAN*ACYCLE, iname);
}

#endif


#if (defined(ASYS_HASOUTPUT) && defined(ASYS_HASINPUT))

/****************************************************************/
/*   sets up audio input and output for a given srate/channels  */
/****************************************************************/

int asys_iosetup(long srate, long ichannels, long ochannels,
		 long isample, long osample, 
		 char * iname, char * oname, long toption)

{

  if (asysi_setup(srate, ichannels, ASYS_ICHAN*ACYCLE, iname) != ASYS_DONE)
    return ASYS_ERROR;
  return asyso_setup(srate, ochannels, ASYS_OCHAN*ACYCLE, oname);

}

#endif

#if defined(ASYS_HASOUTPUT)

/****************************************************************/
/*             shuts down audio output system                   */
/****************************************************************/

void asyso_shutdown(void)

{

  fseek(asyso_fd, 4, SEEK_SET);
  asyso_putlong(asyso_bps*(unsigned long)asyso_nsamp+36,4);
  fseek(asyso_fd, 32, SEEK_CUR);
  asyso_putlong(asyso_bps*(unsigned long)asyso_nsamp,4);
  fclose(asyso_fd);

}

#endif

#if defined(ASYS_HASINPUT)

/****************************************************************/
/*               shuts down audio input system                  */
/****************************************************************/

void asysi_shutdown(void)

{

  fclose(asysi_fd);
}

#endif


#if (defined(ASYS_HASOUTPUT)&&(!defined(ASYS_HASINPUT)))

/****************************************************************/
/*                    shuts down audio output                   */
/****************************************************************/

void asys_oshutdown(void)

{
  asyso_shutdown();
}

#endif

#if (!defined(ASYS_HASOUTPUT)&&(defined(ASYS_HASINPUT)))

/****************************************************************/
/*              shuts down audio input device                   */
/****************************************************************/

void asys_ishutdown(void)

{
  asysi_shutdown();
}

#endif

#if (defined(ASYS_HASOUTPUT)&&(defined(ASYS_HASINPUT)))

/****************************************************************/
/*              shuts down audio input and output device        */
/****************************************************************/

void asys_ioshutdown(void)

{
  asysi_shutdown();
  asyso_shutdown();
}

#endif


#if defined(ASYS_HASOUTPUT)


/****************************************************************/
/*        creates buffer, and generates starting silence        */
/****************************************************************/

int asys_preamble(ASYS_OTYPE * asys_obuf[], long * osize)

{
  *asys_obuf = asyso_buf;
  *osize = asyso_size;
  return ASYS_DONE;
}

/****************************************************************/
/*               sends one frame of audio to output             */
/****************************************************************/

int asys_putbuf(ASYS_OTYPE * asys_obuf[], long * osize)

{
  float * buf = *asys_obuf;
  float fval;
  long val;
  long i = 0;
  long j = 0;

  switch (asyso_bps) {
  case 3:
    while (i < *osize)
      {
	fval = ((float)(pow(2, 23) - 1))*buf[i++];
	val = (long)((fval >= 0.0F) ? (fval + 0.5F) : (fval - 0.5F));
	asyso_cbuf[j++] = (unsigned char) (val & 0x000000FF);
	asyso_cbuf[j++] = (unsigned char)((val >> 8) & 0x000000FF);
	asyso_cbuf[j++] = (unsigned char)((val >> 16) & 0x000000FF);
      }
    break;
  case 2:
    while (i < *osize)
      {
	fval = ((float)(pow(2, 15) - 1))*buf[i++];
	val = (long)((fval >= 0.0F) ? (fval + 0.5F) : (fval - 0.5F));
	asyso_cbuf[j++] = (unsigned char) (val & 0x000000FF);
	asyso_cbuf[j++] = (unsigned char)((val >> 8) & 0x000000FF);
      }
    break;
  case 1:
    while (i < *osize)
      {
	fval = ((float)(pow(2, 7) - 1))*buf[i++];
	asyso_cbuf[j++] = (unsigned char)
	  (128 + ((char)((fval >= 0.0F) ? (fval + 0.5F) : (fval - 0.5F))));
      }
    break;
  }
  
  if (rwrite(asyso_cbuf, sizeof(char), j, asyso_fd) != j)
    return ASYS_ERROR;

  asyso_nsamp += *osize;
  *osize = asyso_size;
  return ASYS_DONE;
}

#endif


#if defined(ASYS_HASINPUT)

/****************************************************************/
/*               get one frame of audio from input              */
/****************************************************************/

int asys_getbuf(ASYS_ITYPE * asys_ibuf[], long * isize)

{
  long i = 0;
  long j = 0;

  if (*asys_ibuf == NULL)
    *asys_ibuf = asysi_buf;
  
  if (asysi_nsamp <= 0)
    {
      *isize = 0;
      return ASYS_DONE;
    }

  *isize = (long)rread(asysi_cbuf, sizeof(unsigned char), asysi_bytes, asysi_fd);

  switch (asysi_bps) {
  case 1:                              /* 8-bit  */
    while (i < *isize)
      {
	asysi_buf[i] = ((float)pow(2, -7))*(((short) asysi_cbuf[i]) - 128);
	i++;
      }
    break;
  case 2:                              /* 9-16 bit */
    *isize = (*isize) / 2;
    while (i < *isize)
      {
	asysi_buf[i] = ((float)pow(2, -15))*((long)(asysi_cbuf[j]) + 
				    (((long)((char)(asysi_cbuf[j+1]))) << 8)); 
	i++;
	j += 2;
      }
    break;
  case 3:                            /* 17-24 bit */
    *isize = (*isize) / 3;
    while (i < *isize)
      {
	asysi_buf[i] = ((float)pow(2, -23))*((long)(asysi_cbuf[j]) + 
				    (((long)(asysi_cbuf[j+1])) << 8) + 
				    (((long)((char) asysi_cbuf[j+2])) << 16));
	i++; 
	j += 3;
      }
    break;
  } 

  asysi_nsamp -= *isize;
  return ASYS_DONE;
}

#endif


#undef ASYS_HASOUTPUT


float ksync() { return 0.0F; }


void ksyncinit(void) { }



#undef NS
#define NS(x) nstate->x
#undef NSP
#define NSP nstate
#undef NT
#define NT(x)  nstate->t[x]
#undef NV
#define NV(x)  nstate->v[x].f
#undef NVI
#define NVI(x)  nstate->v[x].i
#undef NVUI
#define NVUI(x)  nstate->v[x].ui
#undef NVU
#define NVU(x)  nstate->v[x]
#undef NP
#define NP(x)  nstate->v[x].f
#undef NPI
#define NPI(x)  nstate->v[x].i
#undef NPUI
#define NPUI(x)  nstate->v[x].ui

void tone_ipass(struct ninstr_types * nstate)
{
   int i;

memset(&(NV(0)), 0, tone_ENDVAR*sizeof(float));
memset(&(NT(0)), 0, tone_ENDTBL*sizeof(struct tableinfo));

}


#undef NS
#define NS(x) nstate->x
#undef NSP
#define NSP nstate
#undef NT
#define NT(x)  nstate->t[x]
#undef NV
#define NV(x)  nstate->v[x].f
#undef NVI
#define NVI(x)  nstate->v[x].i
#undef NVUI
#define NVUI(x)  nstate->v[x].ui
#undef NVU
#define NVU(x)  nstate->v[x]
#undef NP
#define NP(x)  nstate->v[x].f
#undef NPI
#define NPI(x)  nstate->v[x].i
#undef NPUI
#define NPUI(x)  nstate->v[x].ui

void tone_kpass(struct ninstr_types * nstate)
{

   int i;


}


#undef NS
#define NS(x) nstate->x
#undef NSP
#define NSP nstate
#undef NT
#define NT(x)  nstate->t[x]
#undef NV
#define NV(x)  nstate->v[x].f
#undef NVI
#define NVI(x)  nstate->v[x].i
#undef NVUI
#define NVUI(x)  nstate->v[x].ui
#undef NVU
#define NVU(x)  nstate->v[x]
#undef NP
#define NP(x)  nstate->v[x].f
#undef NPI
#define NPI(x)  nstate->v[x].i
#undef NPUI
#define NPUI(x)  nstate->v[x].ui

void tone_apass(struct ninstr_types * nstate)
{

 if  ( NV(tone_init) ==  0.0F )
 { NV(tone_init) =  1.0F ;
 NV(tone_x) =  0.5F ;
 }
NV(tone_x) = NV(tone_x) -  0.196307F  * NV(tone_y);
 NV(tone_y) = NV(tone_y) +  0.196307F  * NV(tone_x);
 TB(BUS_output_bus + 0) += NV(tone_y);
}


#undef NS
#define NS(x) nstate->x
#undef NSP
#define NSP nstate
#undef NT
#define NT(x)  nstate->t[x]
#undef NV
#define NV(x)  nstate->v[x].f
#undef NVI
#define NVI(x)  nstate->v[x].i
#undef NVUI
#define NVUI(x)  nstate->v[x].ui
#undef NVU
#define NVU(x)  nstate->v[x]
#undef NP
#define NP(x)  nstate->v[x].f
#undef NPI
#define NPI(x)  nstate->v[x].i
#undef NPUI
#define NPUI(x)  nstate->v[x].ui


#undef NS
#define NS(x) 0
#undef NSP
#define NSP NULL
#undef NT
#define NT(x)  gtables[x]
#undef NV
#define NV(x)  global[x].f
#undef NVI
#define NVI(x)  global[x].i
#undef NVUI
#define NVUI(x)  global[x].ui
#undef NVU
#define NVU(x)  global[x]
#undef NP
#define NP(x)  global[x].f
#undef NPI
#define NPI(x)  global[x].i
#undef NPUI
#define NPUI(x)  global[x].ui


#undef NS
#define NS(x) nstate->x
#undef NSP
#define NSP nstate
#undef NT
#define NT(x)  nstate->t[x]
#undef NV
#define NV(x)  nstate->v[x].f
#undef NVI
#define NVI(x)  nstate->v[x].i
#undef NVUI
#define NVUI(x)  nstate->v[x].ui
#undef NVU
#define NVU(x)  nstate->v[x]
#undef NP
#define NP(x)  nstate->v[x].f
#undef NPI
#define NPI(x)  nstate->v[x].i
#undef NPUI
#define NPUI(x)  nstate->v[x].ui



#undef NS
#define NS(x) 0
#undef NSP
#define NSP NULL
#undef NT
#define NT(x)  gtables[x]
#undef NV
#define NV(x)  global[x].f
#undef NVI
#define NVI(x)  global[x].i
#undef NVUI
#define NVUI(x)  global[x].ui
#undef NVU
#define NVU(x)  global[x]
#undef NP
#define NP(x)  global[x].f
#undef NPI
#define NPI(x)  global[x].i
#undef NPUI
#define NPUI(x)  global[x].ui



void system_init(int argc, char **argv)
{

   int i;


   srand(((unsigned int)time(0))|1);
   asys_argc = argc;
   asys_argv = argv;


   memset(&(NV(GBL_STARTVAR)), 0, 
          (GBL_ENDVAR-GBL_STARTVAR)*sizeof(float));
   memset(&(NT(0)), 0, GBL_ENDTBL*sizeof(struct tableinfo));

#undef NS
#define NS(x) nstate->x
#undef NSP
#define NSP nstate
#undef NT
#define NT(x)  nstate->t[x]
#undef NV
#define NV(x)  nstate->v[x].f
#undef NVI
#define NVI(x)  nstate->v[x].i
#undef NVUI
#define NVUI(x)  nstate->v[x].ui
#undef NVU
#define NVU(x)  nstate->v[x]
#undef NP
#define NP(x)  nstate->v[x].f
#undef NPI
#define NPI(x)  nstate->v[x].i
#undef NPUI
#define NPUI(x)  nstate->v[x].ui

   for (busidx=0; busidx<ENDBUS;busidx++)
      TB(busidx)=0.0F;


}


#undef NS
#define NS(x) 0
#undef NSP
#define NSP NULL
#undef NT
#define NT(x)  gtables[x]
#undef NV
#define NV(x)  global[x].f
#undef NVI
#define NVI(x)  global[x].i
#undef NVUI
#define NVUI(x)  global[x].ui
#undef NVU
#define NVU(x)  global[x]
#undef NP
#define NP(x)  global[x].f
#undef NPI
#define NPI(x)  global[x].i
#undef NPUI
#define NPUI(x)  global[x].ui



void effects_init(void)
{



}


#undef NS
#define NS(x) nstate->x
#undef NSP
#define NSP nstate
#undef NT
#define NT(x)  nstate->t[x]
#undef NV
#define NV(x)  nstate->v[x].f
#undef NVI
#define NVI(x)  nstate->v[x].i
#undef NVUI
#define NVUI(x)  nstate->v[x].ui
#undef NVU
#define NVU(x)  nstate->v[x]
#undef NP
#define NP(x)  nstate->v[x].f
#undef NPI
#define NPI(x)  nstate->v[x].i
#undef NPUI
#define NPUI(x)  nstate->v[x].ui


#undef NS
#define NS(x) 0
#undef NSP
#define NSP NULL
#undef NT
#define NT(x)  gtables[x]
#undef NV
#define NV(x)  global[x].f
#undef NVI
#define NVI(x)  global[x].i
#undef NVUI
#define NVUI(x)  global[x].ui
#undef NVU
#define NVU(x)  global[x]
#undef NP
#define NP(x)  global[x].f
#undef NPI
#define NPI(x)  global[x].i
#undef NPUI
#define NPUI(x)  global[x].ui



void shut_down(void)
   {

  if (graceful_exit)
    {
      fprintf(stderr, "\nShutting down system ... please wait.\n");
      fprintf(stderr, "If no termination in 10 seconds, use Ctrl-C or Ctrl-\\ to force exit.\n");
      fflush(stderr);
    }
   asys_putbuf(&asys_obuf, &obusidx);
   asys_oshutdown();

   }

void main_apass(void)

{

  if (s_tone[0].noteon == PLAYING)
   tone_apass(s_tone[0].nstate);

}

int main_kpass(void)

{

  if (s_tone[0].noteon == PLAYING)
   tone_kpass(s_tone[0].nstate);

  return graceful_exit;
}

void main_ipass(void)

{

int i;

  sysidx = &s_tone[0];
  switch(sysidx->noteon) {
   case PLAYING:
   if (sysidx->released)
    {
     if (sysidx->turnoff)
      {
        sysidx->noteon = ALLDONE;
        for (i = 0; i < tone_ENDTBL; i++)
         if (sysidx->nstate->t[i].llmem)
           free(sysidx->nstate->t[i].t);
        sysidx->nstate->iline = NULL;
      }
     else
      {
        sysidx->abstime -= KTIME;
        if (sysidx->abstime < 0.0F)
         {
           sysidx->noteon = ALLDONE;
           for (i = 0; i < tone_ENDTBL; i++)
            if (sysidx->nstate->t[i].llmem)
             free(sysidx->nstate->t[i].t);
           sysidx->nstate->iline = NULL;
         }
        else
         sysidx->turnoff = sysidx->released = 0;
      }
    }
   else
    {
     if (sysidx->turnoff)
      {
       sysidx->released = 1;
      }
     else
      {
        if (sysidx->endtime <= scorebeats)
         {
           if (sysidx->abstime <= 0.0F)
             sysidx->turnoff = sysidx->released = 1;
           else
           {
             sysidx->abstime -= KTIME;
             if (sysidx->abstime < 0.0F)
               sysidx->turnoff = sysidx->released = 1;
           }
         }
        else
          if ((sysidx->abstime < 0.0F) &&
           (1.666667e-2F*tempo*sysidx->abstime + 
               sysidx->endtime <= scorebeats))
            sysidx->turnoff = sysidx->released = 1;
      }
    }
   break;
   case TOBEPLAYED:
    if (sysidx->starttime <= scorebeats)
     {
      sysidx->noteon = PLAYING;
      sysidx->notestate = nextstate;
      sysidx->nstate = &ninstr[nextstate];
      if (sysidx->sdur >= 0.0F)
        sysidx->endtime = scorebeats + sysidx->sdur;
      sysidx->kbirth = kcycleidx;
      ninstr[nextstate].iline = sysidx;
      sysidx->time = (kcycleidx-1)*KTIME;
       oldstate = nextstate;
       nextstate = (nextstate+1) % MAXSTATE;
       while ((oldstate != nextstate) && 
              (ninstr[nextstate].iline != NULL))
         nextstate = (nextstate+1) % MAXSTATE;
       if (oldstate == nextstate)
       {
         nextstate = (nextstate+1) % MAXSTATE;
         while ((oldstate != nextstate) && 
             (ninstr[nextstate].iline->time == 0.0F) &&
             (ninstr[nextstate].iline->noteon == PLAYING))
           nextstate = (nextstate+1) % MAXSTATE;
         ninstr[nextstate].iline->noteon = ALLDONE;
         ninstr[nextstate].iline = NULL;
       }
      tone_ipass(sysidx->nstate);
    }
   break;
   }

}

void main_initpass(void)

{


   if (asys_osetup((int)ARATE, ASYS_OCHAN, ASYS_OTYPENAME,  "output.wav",
ASYS_TIMEOPTION) != ASYS_DONE)
   epr(0,NULL,NULL,"audio output device unavailable");
   endkcycle = kbase + (int) 
      (KRATE*(endtime - scorebase)*(60.0F/tempo));

   if (asys_preamble(&asys_obuf, &asys_osize) != ASYS_DONE)
   epr(0,NULL,NULL,"audio output device unavailable");
   ksyncinit();


}

void main_control(void)

{

}

/*
#    Sfront, a SAOL to C translator    
#    This file: Robust file I/O, termination function
#
# Copyright (c) 1999-2006, Regents of the University of California
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#  Redistributions of source code must retain the above copyright
#  notice, this list of conditions and the following disclaimer.
#
#  Redistributions in binary form must reproduce the above copyright
#  notice, this list of conditions and the following disclaimer in the
#  documentation and/or other materials provided with the distribution.
#
#  Neither the name of the University of California, Berkeley nor the
#  names of its contributors may be used to endorse or promote products
#  derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#    Maintainer: John Lazzaro, lazzaro@cs.berkeley.edu
*/


/* handles termination in case of error */

void epr(int linenum, char * filename, char * token, char * message)

{

  fprintf(stderr, "\nRuntime Error.\n");
  if (filename != NULL)
    fprintf(stderr, "Location: File %s near line %i.\n",filename, linenum);
  if (token != NULL)
    fprintf(stderr, "While executing: %s.\n",token);
  if (message != NULL)
    fprintf(stderr, "Potential problem: %s.\n",message);
  fprintf(stderr, "Exiting program.\n\n");
  exit(-1);

}

/* robust replacement for fread() */

size_t rread(void * ptr, size_t size, size_t nmemb, FILE * stream)

{
  long recv;
  long len;
  long retry;
  char * c;

  /* fast path */

  if ((recv = fread(ptr, size, nmemb, stream)) == nmemb)
    return nmemb;

  /* error recovery */
     
  c = (char *) ptr;
  len = retry = 0;

  do 
    {
      if (++retry > IOERROR_RETRY)
	{
	  len = recv = 0;
	  break;
	}

      if (feof(stream))
	{
	  clearerr(stream);
	  break;
	}

      /* ANSI, not POSIX, so can't look for EINTR/EAGAIN  */
      /* Assume it was one of these and retry.            */

      clearerr(stream);
      len += recv;
      nmemb -= recv;

    }
  while ((recv = fread(&(c[len]), size, nmemb, stream)) != nmemb);

  return (len += recv);

}

/* robust replacement for fwrite() */

size_t rwrite(void * ptr, size_t size, size_t nmemb, FILE * stream)

{
  long recv;
  long len;
  long retry;
  char * c;

  /* fast path */

  if ((recv = fwrite(ptr, size, nmemb, stream)) == nmemb)
    return nmemb;

  /* error recovery */
     
  c = (char *) ptr;
  len = retry = 0;

  do 
    {
      if (++retry > IOERROR_RETRY)
	{
	  len = recv = 0;
	  break;
	}

      /* ANSI, not POSIX, so can't look for EINTR/EAGAIN  */
      /* Assume it was one of these and retry.            */

      len += recv;
      nmemb -= recv;

    }
  while ((recv = fwrite(&(c[len]), size, nmemb, stream)) != nmemb);

  return (len += recv);

}


/*
#    Sfront, a SAOL to C translator    
#    This file: Main loop for runtime
#
# Copyright (c) 1999-2006, Regents of the University of California
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#  Redistributions of source code must retain the above copyright
#  notice, this list of conditions and the following disclaimer.
#
#  Redistributions in binary form must reproduce the above copyright
#  notice, this list of conditions and the following disclaimer in the
#  documentation and/or other materials provided with the distribution.
#
#  Neither the name of the University of California, Berkeley nor the
#  names of its contributors may be used to endorse or promote products
#  derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#    Maintainer: John Lazzaro, lazzaro@cs.berkeley.edu
*/


int main(int argc, char *argv[])

{
  system_init(argc, argv);
  effects_init();
  main_initpass();
  for (kcycleidx=kbase; kcycleidx<=endkcycle; kcycleidx++)
    {
      pass = IPASS;
      scorebeats = scoremult*(kcycleidx - kbase) + scorebase;
      absolutetime = (kcycleidx - 1)*KTIME;
      main_ipass();
      pass = KPASS;
      main_control();
      if (main_kpass())
	break;
      pass = APASS;
      for (acycleidx=0; acycleidx<ACYCLE; acycleidx++)
	{
	  for (busidx=0; busidx<ENDBUS;busidx++)
	    bus[busidx]=0.0F;
	  main_apass();
	  for (busidx=BUS_output_bus; busidx<ENDBUS_output_bus;busidx++)
	    {
	      bus[busidx] = (bus[busidx] >  1.0F) ?  1.0F : bus[busidx];
	      asys_obuf[obusidx++] = (bus[busidx] < -1.0F) ? -1.0F:bus[busidx];
	    }
	  if (obusidx >= asys_osize)
	    {
	      obusidx = 0;
	      if (asys_putbuf(&asys_obuf, &asys_osize))
		{
		  fprintf(stderr,"  Sfront: Audio output device problem\n\n");
		  kcycleidx = endkcycle;
		  break;
		}
	    }
	}
      acycleidx = 0; 
      cpuload = ksync();
    }
  shut_down();
  return 0;
}





