#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <math.h>
#include <sndfile.h>

#define SMPLRT 44100
#define C4_FREQ 261.626
#define D4_FREQ 293.665
#define E4_FREQ 329.628
#define F4_FREQ 349.228
#define G4_FREQ 391.995
#define A4_FREQ 440.000
#define B4_FREQ 493.883
#define C5_FREQ 523.251
#define D5_FREQ 587.330
#define E5_FREQ 659.255
#define F5_FREQ 698.456
#define G5_FREQ 783.991
#define A5_FREQ 880.000
#define B5_FREQ 987.767

#define A0_FREQ 27.5000
#define A0 50
#define A4 98

#define NOTE_BASE_FREQ A0_FREQ
#define NOTE_BASE A0
#define NOTE_LOWEST_HEARABLE 60 //you _can_ hear some below, though, but it's not that nice
#define NOTE_CONCERT_PITCH A4
#define NOTE_PAUSE 0
#define FREQUENCY_MULTIPLIER 1.059463094359295 //2^(1/12) (zwölfte wurzel aus 2)
//int note... CONCERT_PITCH = concert pitch (kammerton a'), +1 = half a note higher, -1 = lower
//note==0 : PAUSE

typedef struct sndsmpl_
{
	SNDFILE *sfp;
	int samplerate; //datenpunkte pro sekunde
} sndsmpl;

typedef struct sbeat_
{
	int z; //zähler
	int n; //nenner
	int blen; //datenpunkte pro takt
} sbeat;

typedef struct m_unit_
{ //musikalische einheit : definierte(s) datei, lied und instrument
	sndsmpl ss;
	sbeat beat;
	void (*instrument)(float*,int,int,double);
	void (*pause)(float*,int,int,double);
} m_unit;

typedef enum notelen_
{
	NL_FULL = 32,
	NL_HALF = 16,
	NL_QUARTER = 8,
	NL_QUAVER = 4,
	NL_SEMIQUAVER = 2,
	NL_DEMISEMIQUAVER = 1
} notelen;



void mknoise(sndsmpl ss, void gen(float*,int,int,double),int len, double tone)
{
	static float *dat = NULL;
	static int datsize = 0;
	if (datsize<len) dat = realloc(dat, len * sizeof(float));
	if (!dat)
	{
		fprintf(stderr, "realloc failed.\n");
		exit(1);
	}
	else datsize = len;
	gen(dat, ss.samplerate, len, tone);
	sf_write_float(ss.sfp, dat, len);
}

void mknoise_t(sndsmpl ss, void gen(float*,int,int,double),double duration, double tone)
{
	int len = (int) (duration * (float) ss.samplerate);
	mknoise(ss, gen, len, tone);
}


void mktone(m_unit mu, notelen nl, int note)
{
	int len = mu.beat.blen / NL_FULL * nl;
	if (note)
	{
		double tone = NOTE_BASE_FREQ * pow(FREQUENCY_MULTIPLIER, (note - NOTE_BASE));
		printf("mktone(mu,%d,%d), freq: %f\n", nl, note, tone);
		mknoise(mu.ss, mu.instrument, len, tone);
	}
	else
		mknoise(mu.ss, mu.pause, len, 0.0);
}

void gen_sine(float *data, int samplerate, int len, double tone)
{
	int k;
	//sine wave: A * sin(w*t+o)
	//A = amplitude (multiplier, not to be used here, since we want to stay in [-1,1])
	//w = angular frequency, which is rad/s. 2*M_PI*[Frequency in Hz]
	//t = time
	//o = offset
	for (k=0;k<len;k++)
		data[k] = sin(2.0 * M_PI * tone * (float) k / samplerate);
}

void gen_pause(float *data, int samplerate, int len, double tone)
{
	int k;
	for (k=0;k<len;k++)
		data[k] = 0.0;
}

void gen_rand(float *data, int samplerate, int len, double tone)
{
	int k;
	for (k=0;k<len;k++)
		data[k] = 2.0 * drand48() - 1.0;
}

void generate_file()
{
	sndsmpl ss;
	ss.samplerate = SMPLRT; //datapoints in one second
	if (ss.samplerate%NL_FULL) ss.samplerate = ss.samplerate - (ss.samplerate%NL_FULL);
	//multiple of a full note length
	
	sbeat beat;
	beat.blen = ss.samplerate; //datapoints in one measure (Takt)
	beat.n = beat.z = 2;
	
	SNDFILE *sfp;
	
	SF_INFO info;
	memset(&info, 0, sizeof(SF_INFO));
	info.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
	info.samplerate = ss.samplerate;
	info.channels = 1;

	sfp = sf_open("brot.wav", SFM_WRITE, &info);
	if (sfp == NULL) { printf("File error\n"); exit(1); }

	sf_set_string(sfp, SF_STR_TITLE, "Mitjas Soundcheck");
	sf_set_string(sfp, SF_STR_COMMENT, "Hello thar! This is some test. Cool?");
	sf_set_string(sfp, SF_STR_SOFTWARE, "lawl!");
	sf_set_string(sfp, SF_STR_COPYRIGHT, "All rights reserved by contentmafia.");

	ss.sfp = sfp;
	m_unit mu;
	mu.ss = ss;
	mu.beat = beat;
	mu.instrument = gen_sine;
	mu.pause = gen_rand;
	
	int k;
	for (k=0;k<20;k++)
	{
		mktone(mu, NL_QUAVER, NOTE_CONCERT_PITCH - 10 + k);
		mktone(mu, NL_SEMIQUAVER, NOTE_PAUSE);
	}
	for (k=20;k>0;k--)
	{
		mktone(mu, NL_QUAVER, NOTE_CONCERT_PITCH - 9 + k);
		mktone(mu, NL_SEMIQUAVER, NOTE_PAUSE);
	}
	
	//
	//gen_sine(data, samplerate, samplerate, C4);
	//sf_write_float(sfp, data, samplerate);
		
		
		
	/*
	double buf[IBUFSIZE],s;
	for (i=0; i<IBUFSIZE; i++)
		buf[i] = 2.0 * drand48() - 1.0;
		
	for (k=i=0; k<5*SMPLRT; k++)
	{
		j = i + 1;
		if (j >= IBUFSIZE) j = 0;
		s = (buf[i] + buf[j]) / 2;
		buf[i] = s;
		sf_write_double(sfp, &s, 1);
		i=j;
	}*/

	sf_close(sfp);
}


int main ()
{
	generate_file();
	return 0;
}
