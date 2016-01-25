/*********************************************************************************
*
*   MIDITONES
*
*  Convert a MIDI file into a bytestream of notes
*
*
*   (C) Copyright 2011,2013,2015,2016 Len Shustek
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of version 3 of the GNU General Public License as
*   published by the Free Software Foundation at http://www.gnu.org/licenses,
*   with Additional Permissions under term 7(b) that the original copyright
*   notice and author attibution must be preserved and under term 7(c) that
*   modified versions be marked as different from the original.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
***********************************************************************************/
/*
* Change log
*  19 January 2011, L.Shustek, V1.0
*     -Initial release.
*  26 February 2011, L. Shustek, V1.1
*     -Expand the documentation generated in the output file.
*     -End the binary output file with an "end of score" command.
*     -Fix bug: Some "stop note" commands were generated too early.
*  04 March 2011, L. Shustek, V1.2
*     -Minor error message rewording.
*  13 June 2011, L. Shustek, V1.3
*     -Add -s2 strategy to try to keep each track on its own tone generator
*      for when there are separate speakers.  This obviously works only when
*      each track is monophonic.  (Suggested by Michal Pustejovsky)
*  20 November 2011, L. Shustek, V1.4
*     -Add -cn option to mask which channels (tracks) to process
*     -Add -kn option to change key
*     Both of these are in support of music-playing on my Tesla Coil.
*  05 December 2011, L. Shustek, V1.5
*     -Fix command line parsing error for option -s1
*     -Display the commandline in the C file output
*     -Change to decimal instead of hex for note numbers in the C file output
*  06 August 2013, L. Shustek, V1.6
*     -Changed to allow compilation and execution in 64-bit environments
*      by using C99 standard intN_t and uintN_t types for MIDI structures,
*      and formatting specifications like "PRId32" instead of "ld".
* 04 April 2015, L. Shustek, V1.7
*     -Made friendlier to other compilers: import source of strlcpy and strlcat,
*      fixed various type mismatches that the LCC compiler didn't fret about.
*      Generate "const" for data initialization for compatibility with Arduino IDE v1.6.x.
* 23 January 2016, D. Blackketter, V1.8
*     -Fix warnings and errors building on Mac OS X via "gcc miditones.c"
* 25 January 2016, D. Blackketter, Paul Stoffregen, V1.9
      -Merge in velocity output option from Arduino/Teensy Audio Library
*/

#define VERSION "1.9"


/*--------------------------------------------------------------------------------
*
*
*                               About MIDITONES
*
*
*  MIDITONES converts a MIDI music file into a much simplified stream of commands,
*  so that a version of the music can be played on a synthesizer having only
*  tone generators without any volume or tone controls.
*
*  Volume ("velocity") and instrument specifications in the MIDI files are discarded.
*  All the tracks are prcoessed and merged into a single time-ordered stream of
*  "note on", "note off", and "delay" commands.
*
*  This was written for the "Playtune" Arduino library, which plays polyphonic music
*  using up to 6 tone generators run by the timers on the processor.  See the separate
*  documentation for Playtune.  But MIDITONES may prove useful for other tone
*  generating systems.
*
*  The output can be either a C-language source code fragment that initializes an
*  array with the command bytestream, or a binary file with the bytestream itself.
*
*  MIDITONES is written in standard ANSI C (plus strlcpy and strlcat functions), and
*  is meant to be executed from the command line.  There is no GUI interface.
*
*  The MIDI file format is complicated, and this has not been tested on a very
*  wide variety of file types.  In particular, we have tested only format type "1",
*  which seems to be what most of them are.  Let me know if you find MIDI files
*  that it won't digest and I'll see if I can fix it.

*  This has been tested only on a little-endian PC, but I think it should work on
*  big-endian processors too.  Note that the MIDI file format is inherently
*  big-endian.
*
*
*  *****  The command line  *****
*
*  To convert a MIDI file called "chopin.mid" into a command bytestream, execute
*
*     miditones chopin
*
*  It will create a file in the same directory called "chopin.c" which contains
*  the C-language statement to intiialize an array called "score" with the bytestream.
*
*
*  The general form for command line execution is this:
*
*     miditones [-p] [-lg] [-lp] [-s1] [-tn] [-b] [-cn] [-kn] <basefilename>
*
*  The <basefilename> is the base name, without an extension, for the input and
*  output files.  It can contain directory path information, or not.
*
*  The input file is the base name with the extension ".mid".  The output filename(s)
*  are the base name with ".c", ".bin", and/or ".log" extensions.
*
*
*  The following command-line options can be specified:
*
*  -p   Only parse the MIDI file;  don't generate an output file.
*       Tracks are processed sequentially instead of being merged into chronological order.
*       This is mostly useful when generating a log to debug MIDI file parsing problems.
*
*  -lp  Log input file parsing information to the <basefilename>.log file
*
*  -lg  Log output bytestream generation information to the <basefilename>.log file
*
*  -sn  Use bytestream generation strategy "n".
*       Two strategies are currently implemented:
*          1: favor track 1 notes instead of all tracks equally
*          2: try to keep each track to its own tone generator
*
*  -tn  Generate the bytestream so that at most n tone generators are used.
*       The default is 6 tone generators, and the maximum is 16.
*       The program will report how many notes had to be discarded because there
*       weren't enough tone generators.  Note that for the Arduino Playtunes
*       library, it's ok to have the bytestream use more tone genreators than
*       exist on your processor because any extra notes will be ignored, although
*       it does make the file bigger than necessary . Of course, too many ignored
*       notes will make the music sound really strange!
*
*  -b   Generate a binary file with the name <basefilename>.bin, instead of a
*       C-language source file with the name <basefilename>.c.
*
*  -cn  Only process the channel numbers whose bits are on in the number "n".
*       For example, -c3 means "only process channels 0 and 1"
*
*  -kn  Change the musical key of the output by n chromatic notes.
*       -k-12 goes one octave down, -k12 goes one octave up, etc.
*
*  -v   Add velocity information to output
*
*
*  *****  The score bytestream  *****
*
*  The generated bytestream is a series of commands that turn notes on and off, and
*  start delays until the next note change.  Here are the details, with numbers
*  shown in hexadecimal.
*
*  If the high-order bit of the byte is 1, then it is one of the following commands:
*
*    9t nn [vv] Start playing note nn on tone generator t.  Generators are numbered
*           starting with 0.  The notes numbers are the MIDI numbers for the chromatic
*           scale, with decimal 60 being Middle C, and decimal 69 being Middle A (440 Hz).
*           if the -v option is enabled, a second byte is added to indicate velocity
*
*    8t     Stop playing the note on tone generator t.
*
*    F0     End of score: stop playing.
*
*    E0     End of score: start playing again from the beginning.
*           (Shown for completeness; MIDITONES won't generate this.)
*
*  If the high-order bit of the byte is 0, it is a command to delay for a while until
*  the next note change..  The other 7 bits and the 8 bits of the following byte are
*  interpreted as a 15-bit big-endian integer that is the number of milliseconds to
*  wait before processing the next command.  For example,
*
*     07 D0
*
*  would cause a delay of 0x07d0 = 2000 decimal millisconds, or 2 seconds.  Any tones
*  that were playing before the delay command will continue to play.
*
*
*  Len Shustek, 4 Feb 2011
*
*----------------------------------------------------------------------------------*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <time.h>
#include <inttypes.h>


/***********  MIDI file header formats  *****************/

struct midi_header {
    int8_t MThd[4];
    uint32_t header_size;
    uint16_t format_type;
    uint16_t number_of_tracks;
    uint16_t time_division;
};

struct track_header {
    int8_t MTrk[4];
    uint32_t track_size;
};


/***********  Global variables  ******************/

#define MAX_TONEGENS 16		/* max tone generators: tones we can play simultaneously */
#define DEFAULT_TONEGENS 6	/* default number of tone generators */
#define MAX_TRACKS 24		/* max number of MIDI tracks we will process */

bool loggen, logparse, parseonly, strategy1, strategy2, binaryoutput, velocityoutput;
FILE *infile, *outfile, *logfile;
uint8_t *buffer, *hdrptr;
unsigned long buflen;
int num_tracks;
int tracks_done = 0;
int outfile_itemcount = 0;
int num_tonegens = DEFAULT_TONEGENS;
int num_tonegens_used = 0;
unsigned channel_mask = 0xffff; // bit mask of channels to process
int keyshift = 0;  // optional chromatic note shift for output file
long int outfile_bytecount = 0;
unsigned int ticks_per_beat = 240;
unsigned long timenow = 0;
unsigned long tempo;	/* current tempo in usec/qnote */

struct tonegen_status {	/* current status of a tone generator */
    bool playing;	/* is it playing? */
    int track;		/* if so, which track is the note from? */
    int note;		/* what note is playing? */
}
tonegen [MAX_TONEGENS] = {
    {0}};

struct track_status {	/* current processing point of a MIDI track */
    uint8_t		 *trkptr;		/* ptr to the next note change */
    uint8_t		 *trkend;		/* ptr past the end of the track */
    unsigned long time;			/* what time we're at in the score */
    unsigned long tempo;		/* the tempo last set, in usec/qnote */
    unsigned int preferred_tonegen;	/* for strategy2: try to use this generator */
    unsigned char cmd;			/* CMD_xxxx  next to do */
    unsigned char note;			/* for which note */
    unsigned char velocity;
    unsigned char last_event;	/* the last event, for MIDI's "running status" */
    bool tonegens[MAX_TONEGENS];/* which tone generators our notes are playing on */
}
track[MAX_TRACKS] = {
    {0}};


/* output bytestream commands, which are also stored in track_status.cmd */

#define CMD_PLAYNOTE	0x90	/* play a note: low nibble is generator #, note is next byte */
#define CMD_STOPNOTE	0x80	/* stop a note: low nibble is generator # */
#define CMD_RESTART	0xe0	/* restart the score from the beginning */
#define CMD_STOP		0xf0	/* stop playing */
/* if CMD < 0x80, then the other 7 bits and the next byte are a 15-bit number of msec to delay */

/* these other commands stored in the track_status.com */
#define CMD_TEMPO		0xFE	/* tempo in usec per quarter note ("beat") */
#define CMD_TRACKDONE	0xFF	/* no more data left in this track */



/**************  command-line processing  *******************/

void SayUsage(char *programName){
    static char *usage[] = {
        "Convert MIDI files to an Arduino PLAYTUNE bytestream",
        "miditones [-p] [-lg] [-lp] [-s1] [-tn] [-v] <basefilename>",
        "  -p   parse only, don't generate bytestream",
        "  -lp  log input parsing",
        "  -lg  log output generation",
        "  -s1  strategy 1: favor track 1",
        "  -s2  strategy 2: try to assign tracks to specific tone generators",
        "  -tn  use at most n tone generators (default is 6, max is 16)",
        "  -b   binary file output instead of C source text",
        "  -cn  mask for which tracks to process, e.g. -c3 for only 0 and 1",
        "  -kn  key shift in chromatic notes, positive or negative",
        "  -v   include velocity data in play note commands",
        "input file:  <basefilename>.mid",
        "output file: <basefilename>.bin or .c",
        "log file:    <basefilename>.log",
        ""							};
    int i=0;
    while (usage[i][0] != '\0') fprintf(stderr, "%s\n", usage[i++]);
}

int HandleOptions(int argc,char *argv[]) {
    /* returns the index of the first argument that is not an option; i.e.
    does not start with a dash or a slash*/

    int i,firstnonoption=0;

    /* --- The following skeleton comes from C:\lcc\lib\wizard\textmode.tpl. */
    for (i=1; i< argc;i++) {
        if (argv[i][0] == '/' || argv[i][0] == '-') {
            switch (toupper(argv[i][1])) {
            case 'H':
            case '?':
                SayUsage(argv[0]);
                exit(1);
            case 'L':
                if (toupper(argv[i][2]) == 'G') loggen = true;
                else if (toupper(argv[i][2]) == 'P') logparse = true;
                else goto opterror;
                break;
            case 'P':
                parseonly = true;
                break;
            case 'B':
                binaryoutput = true;
                break;
            case 'V':
                velocityoutput = true;
                break;
            case 'S':
                if (argv[i][2] == '1') strategy1 = true;
                else if (argv[i][2] == '2') strategy2 = true;
                else goto opterror;
                break;
            case 'T':
                if (sscanf(&argv[i][2],"%d",&num_tonegens) != 1 || num_tonegens <1 || num_tonegens > MAX_TONEGENS) goto opterror;
                printf("Using %d tone generators.\n", num_tonegens);
                break;
            case 'C':
                if (sscanf(&argv[i][2],"%d",&channel_mask) != 1 || channel_mask > 0xffff) goto opterror;
                printf("Channel (track) mask is %04X.\n", channel_mask);
                break;
            case 'K':
            if (sscanf(&argv[i][2],"%d",&keyshift) != 1 || keyshift < -100 || keyshift > 100) goto opterror;
                printf("Using keyshift %d.\n", keyshift);
                break;

                /* add more  option switches here */
opterror:
            default:
                fprintf(stderr,"unknown option: %s\n",argv[i]);
                SayUsage(argv[0]);
                exit(4);
            }
        }
        else {
            firstnonoption = i;
            break;
        }
    }
    return firstnonoption;
}

void print_command_line (int argc,char *argv[]) {
    int i;
	fprintf(outfile, "// command line: ");
    for (i=0; i< argc; i++) fprintf(outfile,"%s ",argv[i]);
    fprintf(outfile, "\n");
}


/****************  utility routines  **********************/


/* safe string copy */
size_t miditones_strlcpy(char *dst, const char *src,	size_t 	siz) {
	char       *d = dst;
	const char *s = src;
	size_t      n = siz;
	/* Copy as many bytes as will fit */
	if (n != 0)
	{
		while (--n != 0)
		{
			if ((*d++ = *s++) == '\0')
				break;
		}
	}
	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0)
	{
		if (siz != 0)
			*d = '\0';          /* NUL-terminate dst */
		while (*s++)
			;
	}
	return (s - src - 1);       /* count does not include NUL */
}

/* safe string concatenation */

size_t miditones_strlcat(char *dst, const char *src, size_t siz) {
	char       *d = dst;
	const char *s = src;
	size_t      n = siz;
	size_t      dlen;
	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0 && *d != '\0')
		d++;
	dlen = d - dst;
	n = siz - dlen;
	if (n == 0)
		return (dlen + strlen(s));
	while (*s != '\0')
	{
		if (n != 1)
		{
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';
	return (dlen + (s - src));  /* count does not include NUL */
}

/* match a constant character sequence */

int charcmp (const char *buf, const char *match) {
    int len, i;
    len = strlen (match);
    for (i=0; i<len; ++i)
        if (buf[i] != match[i])	return 0;
    return 1;
}

/*	announce a fatal MIDI file format error */

void midi_error (char *msg, unsigned char *bufptr) {
    unsigned char *ptr;
    fprintf(stderr, "---> MIDI file error at position %04X (%d): %s\n", (uint16_t)(bufptr-buffer), (uint16_t)(bufptr-buffer), msg);
    /* print some bytes surrounding the error */
    ptr = bufptr - 16;
    if (ptr < buffer) ptr = buffer;
    for (; ptr <= bufptr+16 && ptr < buffer+buflen; ++ptr) fprintf (stderr, ptr==bufptr ? " [%02X]  ":"%02X ", *ptr);
    fprintf(stderr, "\n");
    exit(8);
}

/* check that we have a specified number of bytes left in the buffer */

void chk_bufdata(unsigned char *ptr, unsigned long int len) {
    if ((unsigned)(ptr + len - buffer) > buflen) midi_error("data missing", ptr);
}


/* fetch big-endian numbers */

uint16_t rev_short (uint16_t val) {
    return ((val&0xff)<<8) | ((val>>8)&0xff);
}

uint32_t rev_long (uint32_t val){
    return (((rev_short((uint16_t)val) & 0xffff) << 16) |
            (rev_short((uint16_t)(val >> 16)) & 0xffff));
}

/* account for new items in the non-binary output file
and generate a newline every so often. */

void outfile_items (int n) {
    outfile_bytecount += n;
    outfile_itemcount += n;
    if (!binaryoutput && outfile_itemcount > 20) {
        fprintf (outfile, "\n");
        outfile_itemcount = 0;
    }
}

/**************  process the MIDI file header  *****************/

void process_header (void) {
    struct midi_header *hdr;
    unsigned int time_division;

    chk_bufdata(hdrptr, sizeof(struct midi_header));
    hdr = (struct midi_header *) hdrptr;
    if (!charcmp((char*)hdr->MThd,"MThd")) midi_error("Missing 'MThd'", hdrptr);

    num_tracks = rev_short(hdr->number_of_tracks);

    time_division = rev_short(hdr->time_division);
    if (time_division < 0x8000)	ticks_per_beat = time_division;
    else ticks_per_beat = ((time_division >> 8) & 0x7f) /* SMTE frames/sec */ * (time_division & 0xff); /* ticks/SMTE frame */

    if (logparse) {
        fprintf (logfile, "Header size %" PRId32 "\n", rev_long(hdr->header_size));
        fprintf (logfile, "Format type %d\n", rev_short(hdr->format_type));
        fprintf (logfile, "Number of tracks %d\n", num_tracks);
        fprintf (logfile, "Time division %04X\n", time_division);
        fprintf (logfile, "Ticks/beat = %d\n", ticks_per_beat);

    }
    hdrptr += rev_long(hdr->header_size) + 8;  /* point past header to track header, presumably. */
    return;
}


/****************  Process a MIDI track header *******************/

void start_track (int tracknum) {
    struct track_header *hdr;
    unsigned long tracklen;

    chk_bufdata(hdrptr, sizeof(struct track_header));
    hdr = (struct track_header *) hdrptr;
    if (!charcmp((char *)(hdr->MTrk),"MTrk")) midi_error("Missing 'MTrk'", hdrptr);

    tracklen = rev_long(hdr->track_size);
    if (logparse) fprintf (logfile, "\nTrack %d length %ld\n", tracknum, tracklen);
    hdrptr += sizeof (struct track_header);  /* point past header */
    chk_bufdata(hdrptr, tracklen);
    track[tracknum].trkptr = hdrptr;
    hdrptr += tracklen;  /* point to the start of the next track */
    track[tracknum].trkend = hdrptr;  /* the point past the end of the track */
}


/* Get a MIDI-style variable-length integer */

unsigned long get_varlen (uint8_t **ptr) {
    /* Get a 1-4 byte variable-length value and adjust the pointer past it.
    These are a succession of 7-bit values with a MSB bit of zero marking the end */

    unsigned long val;
    int i, byte;

    val = 0;
    for (i=0; i<4;  ++i){
        byte = *(*ptr)++;
        val = (val<<7) | (byte&0x7f);
        if (!(byte&0x80)) return val;
    }
    return val;
}


/***************  Process the MIDI track data  ***************************/

/* Skip in the track for the next "note on", "note off" or "set tempo" command,
then record that information in the track status block and return. */

void find_note (int tracknum) {
    unsigned long int delta_time;
    int event, chan;
    int i;
    int note, velocity;
    int meta_cmd, meta_length;
    unsigned long int sysex_length;
    struct track_status *t;

    /* process events */

    t = &track[tracknum]; /* our track status structure */
    while (t->trkptr < t->trkend) {

        delta_time = get_varlen(&t->trkptr);
        if (logparse) {
            fprintf (logfile, "trk %d ", tracknum);
            if (delta_time) {
              fprintf (logfile, "delta time %4ld, ", delta_time);
            } else {
              fprintf (logfile, "                ");
            }
        }
        t->time += delta_time;

        if (*t->trkptr < 0x80) /* "running status" */ event = t->last_event;/* means same event as before */
        else { /* new "status" (event type) */
            event = *t->trkptr++;
            t->last_event = event;
        }

        if (event == 0xff) { /* meta-event */
            meta_cmd = *t->trkptr++;
            meta_length = *t->trkptr++;
            switch (meta_cmd) {
            case 0x2f:
                if (logparse) fprintf(logfile, "end of track\n");
                break;
            case 0x00:
                if (logparse) fprintf(logfile, "sequence number %d\n", rev_short(*(unsigned short *)t->trkptr));
                break;
            case 0x20:
                if (logparse) fprintf(logfile, "channel prefix %d\n", *t->trkptr);
                break;
            case 0x51: 	/* tempo: 3 byte big-endian integer! */
                t->cmd = CMD_TEMPO;
                t->tempo = rev_long(*(unsigned long *)(t->trkptr-1)) & 0xffffffL;
                if (logparse) fprintf(logfile, "set tempo %ld usec/qnote\n", t->tempo);
                t->trkptr += meta_length;
                return;
            case 0x54:
                if (logparse) fprintf(logfile, "SMPTE offset %08" PRIx32 "\n", rev_long(*(unsigned long *)t->trkptr));
                break;
            case 0x58:
                if (logparse) fprintf(logfile, "time signature %08" PRIx32 "\n", rev_long(*(unsigned long *)t->trkptr));
                break;
            case 0x59:
                if (logparse) fprintf(logfile, "key signature %04X\n", rev_short(*(unsigned short *)t->trkptr));
                break;
            default: /* assume it is a string */
                if (logparse) {
                    fprintf(logfile, "meta cmd %02X, length %d, \"", meta_cmd, meta_length);
                    for (i=0; i<meta_length; ++i) {
                        int ch = t->trkptr[i];
                        fprintf(logfile, "%c", isprint(ch) ? ch : '?');
                    }
                    fprintf(logfile, "\"\n");
                }
                if (tracknum==0 && meta_cmd==0x03 && !parseonly && !binaryoutput) {
                    /* Incredibly, MIDI has no standard for recording the name of the piece!
                    Track 0's "trackname" (meta 0x03) is sometimes used for that, so
                    we output it to the C file as documentation. */
                    fprintf(outfile, "// ");
                    for (i=0; i<meta_length; ++i) {
                        int ch = t->trkptr[i];
                        fprintf(outfile, "%c", isprint(ch) ? ch : '?');
                    }
                    fprintf(outfile, "\n");
                }
                break;
            }
            t->trkptr += meta_length;
        }

        else if (event <0x80) midi_error("Unknown MIDI event type", t->trkptr);

        else {
            chan = event & 0xf;
            switch (event>>4) {
            case 0x8:
                t->note = *t->trkptr++;
                velocity = *t->trkptr++;
note_off:
                 if (logparse) fprintf (logfile, "note %02X off, chan %d, velocity %d\n", t->note, chan, velocity);
                 if ((1<<chan) & channel_mask) { // if we're processing this channel
                    t->cmd = CMD_STOPNOTE;
                    return;  /* stop processing and return */
                }
                break; // else keep looking
            case 0x9:
                t->note = *t->trkptr++;
                velocity = *t->trkptr++;
                if (velocity == 0)  /* some scores use note-on with zero velocity for off! */	goto note_off;
                t->velocity = velocity;
                if (logparse) fprintf (logfile, "note %02X on,  chan %d, velocity %d\n", t->note, chan, velocity);
                if ((1<<chan) & channel_mask) {  // if we're processing this channel
                    t->cmd = CMD_PLAYNOTE;
                    return;  /* stop processing and return */
                }
                break; // else keep looking
            case 0xa:
                note = *t->trkptr++;
                velocity = *t->trkptr++;
                if (logparse) fprintf (logfile, "after-touch %02X, %02X\n", note, velocity);
                break;
            case 0xb:
                note = *t->trkptr++;
                velocity = *t->trkptr++;
                if (logparse) fprintf (logfile, "control change %02X, %02X\n", note, velocity);
                break;
            case 0xc:
                note = *t->trkptr++;
                if (logparse) fprintf(logfile, "program patch %02X\n", note);
                break;
            case 0xd:
                chan = *t->trkptr++;
                if (logparse) fprintf(logfile, "channel after-touch %02X\n", chan);
                break;
            case 0xe:
                note = *t->trkptr++;
                velocity = *t->trkptr++;
                if (logparse) fprintf(logfile, "pitch wheel change %02X, %02X\n", note, velocity);
                break;
            case 0xf:
                sysex_length = get_varlen(&t->trkptr);
                if (logparse) fprintf(logfile, "SysEx event %02X, %ld bytes\n", event, sysex_length);
                t->trkptr += sysex_length;
                break;
            default:
                midi_error("Unknown MIDI command", t->trkptr);
            }
        }
    }
    t->cmd = CMD_TRACKDONE;  /* no more notes to process */
    ++tracks_done;
}


/*********************  main  ****************************/

int main(int argc,char *argv[]) {
    int argno;
    char *filebasename;
#define MAXPATH 120
    char filename[MAXPATH];

    int tracknum;
    int earliest_tracknum;
    unsigned long earliest_time;
    int notes_skipped = 0;

    printf("MIDITONES V%s, (C) 2011,2015,2016 Len Shustek\n", VERSION);
    printf("See the source code for license information.\n\n");
    if (argc == 1) { /* no arguments */
        SayUsage(argv[0]);
        return 1;
    }

    /* process options */

    argno = HandleOptions(argc,argv);
    filebasename = argv[argno];

    /* Open the log file */

    if (logparse || loggen) {
        miditones_strlcpy(filename, filebasename, MAXPATH);
        miditones_strlcat(filename, ".log", MAXPATH);
        logfile = fopen(filename, "w");
        if (!logfile) {
            fprintf(stderr, "Unable to open log file %s", filename);
            return 1;
        }
    }

    /* Open the input file */

    miditones_strlcpy(filename, filebasename, MAXPATH);
    miditones_strlcat(filename, ".mid", MAXPATH);
    infile = fopen(filename, "rb");
    if (!infile) {
        fprintf(stderr, "Unable to open input file %s", filename);
        return 1;
    }

    /* Read the whole input file into memory */

    fseek(infile, 0, SEEK_END); /* find file size */
    buflen = ftell(infile);
    fseek(infile, 0, SEEK_SET);

    buffer = (unsigned char *) malloc (buflen+1);
    if (!buffer) {
        fprintf(stderr, "Unable to allocate %ld bytes for the file", buflen);
        return 1;
    }

    fread(buffer, buflen, 1, infile);
    fclose(infile);
    if (logparse) fprintf(logfile, "Processing %s, %ld bytes\n", filename, buflen);

    /* Create the output file */

    if (!parseonly) {
        miditones_strlcpy(filename, filebasename, MAXPATH);
        if (binaryoutput) {
            miditones_strlcat(filename, ".bin", MAXPATH);
            outfile = fopen(filename, "wb");
        }
        else {
            miditones_strlcat(filename, ".c", MAXPATH);
            outfile = fopen(filename, "w");
        }
        if (!outfile) {
            fprintf(stderr, "Unable to open output file %s", filename);
            return 1;
        }
        if (!binaryoutput) {  /* create header of C file that initializes score data */
            time_t rawtime;
            time (&rawtime);
            fprintf(outfile, "// Playtune bytestream for file \"%s.mid\" ", filebasename);
            fprintf(outfile, "created by MIDITONES V%s on %s", VERSION, asctime(localtime(&rawtime)));
            print_command_line(argc,argv);
            if (channel_mask != 0xffff)
                fprintf(outfile, "//   Only the masked channels were processed: %04X\n", channel_mask);
            if (keyshift != 0)
                fprintf(outfile, "//   Keyshift was %d chromatic notes\n", keyshift);
            fprintf(outfile, "#ifdef __AVR__\n");
            fprintf(outfile, "#include <avr/pgmspace.h>\n");
            fprintf(outfile, "#else\n");
            fprintf(outfile, "#define PROGMEM\n");
            fprintf(outfile, "#endif\n");
            fprintf(outfile, "const unsigned char PROGMEM score [] = {\n");
        }
    }

    /* process the MIDI file header */

    hdrptr = buffer;	/* pointer to file and track headers */
    process_header ();
    printf ("  Processing %d tracks.\n", num_tracks);
    if (num_tracks > MAX_TRACKS) midi_error ("Too many tracks", buffer);

    /* initialize processing of all the tracks */

    for (tracknum=0; tracknum < num_tracks; ++tracknum) {
        start_track (tracknum); /* process the track header */
        find_note (tracknum);	/* position to the first note on/off */
        /* if we are in "parse only" mode, do the whole track,
        so we do them one at a time instead of time-synchronized. */
        if (parseonly) while (track[tracknum].cmd != CMD_TRACKDONE) find_note(tracknum);
    }

    /* Continue processing all tracks, in an order based on the simulated time.
    This is not unlike multiway merging used for tape sorting algoritms in the 50's! */

    tracknum = 0;
    if (!parseonly) do { /* while there are still track notes to process */
        struct track_status *trk;
        struct tonegen_status *tg;
        int tgnum;
        int count_tracks;
        unsigned long delta_time, delta_msec;

        /* Find the track with the earliest event time,
        and output a delay command if time has advanced.

        A potential improvement: If there are multiple tracks with the same time,
        first do the ones with STOPNOTE as the next command, if any.  That would
        help avoid running out of tone generators.  In practice, though, most MIDI
        files do all the STOPNOTEs first anyway, so it won't have much effect.
        */

        earliest_time = 0x7fffffff;

        /* Usually we start with the track after the one we did last time (tracknum),
        so that if we run out of tone generators, we have been fair to all the tracks.
        The alternate "strategy1" says we always start with track 0, which means
        that we favor early tracks over later ones when there aren't enough tone generators.
        */

        count_tracks = num_tracks;
        if (strategy1) tracknum = num_tracks; /* beyond the end, so we start with track 0 */
        do {
            if (++tracknum >= num_tracks) tracknum=0;
            trk = &track[tracknum];
            if (trk->cmd != CMD_TRACKDONE && trk->time < earliest_time) {
                earliest_time = trk->time;
                earliest_tracknum = tracknum;
            }
        }
        while (--count_tracks);

        tracknum = earliest_tracknum;  /* the track we picked */
        trk = &track[tracknum];

        if (loggen) fprintf (logfile, "Earliest time is trk %d, time %ld\n", tracknum, earliest_time);
        if (earliest_time < timenow) midi_error ("INTERNAL: time went backwards", trk->trkptr);

        /* If time has advanced, output a "delay" command */

        delta_time = earliest_time - timenow;
        if (delta_time) {
            /* Convert ticks to milliseconds based on the current tempo */
            delta_msec = ((unsigned long) delta_time * tempo) / ticks_per_beat / 1000;
            if (loggen) fprintf (logfile, "->Delay %ld msec (%ld ticks)\n", delta_msec, delta_time);
            if (delta_msec > 0x7fff) midi_error ("INTERNAL: time delta too big", trk->trkptr);
            /* output a 15-bit delay in big-endian format */
            if (binaryoutput) {
                putc ((unsigned char) (delta_msec >> 8), outfile);
                putc ((unsigned char) (delta_msec & 0xff), outfile);
                outfile_bytecount += 2;
            }
            else 	{
                fprintf (outfile, "%ld,%ld, ", delta_msec >> 8, delta_msec & 0xff);
                outfile_items(2);
            }
        }
        timenow = earliest_time;

        /*	If this track event is "set tempo", just change the global tempo.
        That affects how we generate "delay" commands. */

        if (trk->cmd == CMD_TEMPO) {
            tempo = trk->tempo;
            if (loggen) fprintf (logfile, "Tempo changed to %ld usec/qnote\n", tempo);
            find_note (tracknum);
        }

        /*	If this track event is "stop note", process it and all subsequent "stop notes" for this track
        that are happening at the same time. Doing so frees up as many tone generators as possible.  */

        else if (trk->cmd == CMD_STOPNOTE) do {

            // stop a note
            for (tgnum=0; tgnum < num_tonegens; ++tgnum) {  /* find which generator is playing it */
                tg = &tonegen[tgnum];
                if (tg->playing && tg->track == tracknum && tg->note == trk->note) {
                    if (loggen) fprintf (logfile, "->Stop note %02X, generator %d, track %d\n", tg->note, tgnum, tracknum);
                    if (binaryoutput) {
                        putc (CMD_STOPNOTE | tgnum, outfile);
                        outfile_bytecount += 1;
                    }
                    else {
                        fprintf (outfile, "0x%02X, ", CMD_STOPNOTE | tgnum);
                        outfile_items (1);
                    }
                    tg->playing = false;
                    trk->tonegens[tgnum] = false;
                }
            }
            find_note (tracknum);  // use up the note
        }
        while (trk->cmd == CMD_STOPNOTE && trk->time == timenow);

        /*	If this track event is "start note", process only it.
        Don't do more than one, so we allow other tracks their chance at grabbing tone generators. */

        else if (trk->cmd == CMD_PLAYNOTE) {
            bool foundgen = false;

            if (strategy2) { /* try to use the same tone generator this track used last time */
                tg = &tonegen [trk->preferred_tonegen];
                if (!tg->playing) {
                    tgnum = trk->preferred_tonegen;
                    foundgen = true;
                }
            }
            if (!foundgen) for (tgnum=0; tgnum < num_tonegens; ++tgnum) {  /* search for a free tone generator */
                tg = &tonegen[tgnum];
                if (!tg->playing) {
                    foundgen = true;
                    break;
                }
            }
            if (foundgen) {
                int shifted_note;
                if (tgnum+1 > num_tonegens_used) num_tonegens_used = tgnum+1;
                    tg->playing = true;
                    tg->track = tracknum;
                    tg->note = trk->note;
                    trk->tonegens[tgnum] = true;
                    trk->preferred_tonegen = tgnum;
                    if (loggen) fprintf (logfile, "->Start note %02X, generator %d, track %d\n", trk->note, tgnum, tracknum);
                    shifted_note = trk->note + keyshift;
                    if (shifted_note < 0) shifted_note = 0;
                    if (shifted_note > 127) shifted_note = 127;
                    if (binaryoutput) {
                        putc (CMD_PLAYNOTE | tgnum, outfile);
                        putc (shifted_note, outfile);
                        outfile_bytecount += 2;
                        if (velocityoutput) {
                          putc (trk->velocity, outfile);
                          outfile_bytecount++;
                        }
                    }
                    else {
                        if (velocityoutput == 0) {
                          fprintf (outfile, "0x%02X,%d, ", CMD_PLAYNOTE | tgnum, shifted_note);
                          outfile_items(2);
                        } else {
                          fprintf (outfile, "0x%02X,%d,%d, ", CMD_PLAYNOTE | tgnum, shifted_note, trk->velocity);
                          outfile_items(3);
                        }
                    }
            }
            else {
                if (loggen) fprintf (logfile, "----> No free generator, skipping note %02X, track %d\n", trk->note, tracknum);
                ++notes_skipped;
            }
            find_note (tracknum);	// use up the note
        }

    } /* !parseonly do */
    while (tracks_done < num_tracks);

    if (!parseonly) {
        // generate the end-of-score command and some commentary
        if(binaryoutput) putc(CMD_STOP, outfile);
        else {
            fprintf(outfile, "0x%02x};\n// This score contains %ld bytes, and %d tone generator%s used.\n", CMD_STOP, outfile_bytecount, num_tonegens_used, num_tonegens_used == 1 ? " is" : "s are");
            if (notes_skipped) fprintf(outfile, "// %d notes had to be skipped.\n", notes_skipped);
        }
        printf ("  %s %d tone generators were used.\n", num_tonegens_used < num_tonegens ? "Only":"All", num_tonegens_used);
        if (notes_skipped) printf("  %d notes were skipped because there weren't enough tone generators.\n", notes_skipped);
        printf ("  %ld bytes of score data were generated.\n", outfile_bytecount);
    }


    printf ("  Done.\n");
    return 0;
}
