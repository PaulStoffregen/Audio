#include "AudioStream.h"


// When changing multiple audio object settings that must update at
// the same time, these functions allow the audio library interrupt
// to be disabled.  For example, you may wish to begin playing a note
// in response to reading an analog sensor.  If you have "velocity"
// information, you might start the sample playing and also adjust
// the gain of a mixer channel.  Use AudioNoInterrupts() first, then
// make both changes to the 2 separate objects.  Then allow the audio
// library to update with AudioInterrupts().  Both changes will happen
// at the same time, because AudioNoInterrupts() prevents any updates
// while you make changes.
#define AudioNoInterrupts() (NVIC_DISABLE_IRQ(IRQ_SOFTWARE))
#define AudioInterrupts()   (NVIC_ENABLE_IRQ(IRQ_SOFTWARE))


// waveforms.c
extern "C" {
extern const int16_t AudioWaveformSine[257];
extern const int16_t AudioWaveformTriangle[257];
extern const int16_t AudioWaveformSquare[257];
extern const int16_t AudioWaveformSawtooth[257];
}

// windows.c
extern "C" {
extern const int16_t AudioWindowHanning256[];
extern const int16_t AudioWindowBartlett256[];
extern const int16_t AudioWindowBlackman256[];
extern const int16_t AudioWindowFlattop256[];
extern const int16_t AudioWindowBlackmanHarris256[];
extern const int16_t AudioWindowNuttall256[];
extern const int16_t AudioWindowBlackmanNuttall256[];
extern const int16_t AudioWindowWelch256[];
extern const int16_t AudioWindowHamming256[];
extern const int16_t AudioWindowCosine256[];
extern const int16_t AudioWindowTukey256[];
}

class AudioAnalyzeFFT256 : public AudioStream
{
public:
	AudioAnalyzeFFT256(uint8_t navg = 8, const int16_t *win = AudioWindowHanning256)
	  : AudioStream(1, inputQueueArray), outputflag(false),
	    prevblock(NULL), count(0), naverage(navg), window(win)  { init(); }

	bool available() {
		if (outputflag == true) {
			outputflag = false;
			return true;
		}
		return false;
	}
	virtual void update(void);
	//uint32_t cycles;
	int32_t output[128] __attribute__ ((aligned (4)));
private:
	void init(void);
	const int16_t *window;
	audio_block_t *prevblock;
	int16_t buffer[512] __attribute__ ((aligned (4)));
	uint8_t count;
	uint8_t naverage;
	bool outputflag;
	audio_block_t *inputQueueArray[1];
};



class AudioSynthWaveform : public AudioStream
{
public:
	AudioSynthWaveform(const int16_t *waveform)
	  : AudioStream(0, NULL), wavetable(waveform), magnitude(0), phase(0) { }
	void frequency(float freq) {
		if (freq > AUDIO_SAMPLE_RATE_EXACT / 2 || freq < 0.0) return;
		phase_increment = (freq / AUDIO_SAMPLE_RATE_EXACT) * 4294967296.0f;
	}
	void amplitude(float n) {        // 0 to 1.0
		if (n < 0) n = 0;
		else if (n > 1.0) n = 1.0;
		magnitude = n * 32767.0;
	}
	virtual void update(void);
private:
	const int16_t *wavetable;
	uint16_t magnitude;
	uint32_t phase;
	uint32_t phase_increment;
};




#if 0
class AudioSineWaveMod : public AudioStream
{
public:
	AudioSineWaveMod() : AudioStream(1, inputQueueArray) {}
	void frequency(float freq);
	//void amplitude(q15 n);
	virtual void update(void);
private:
	uint32_t phase;
	uint32_t phase_increment;
	uint32_t modulation_factor;
	audio_block_t *inputQueueArray[1];
};
#endif





class AudioOutputPWM : public AudioStream
{
public:
	AudioOutputPWM(void) : AudioStream(1, inputQueueArray) { begin(); }
	virtual void update(void);
	void begin(void);
	friend void dma_ch3_isr(void);
private:
	static audio_block_t *block_1st;
	static audio_block_t *block_2nd;
	static uint32_t block_offset;
	static bool update_responsibility;
	static uint8_t interrupt_count;
	audio_block_t *inputQueueArray[1];
};





class AudioOutputAnalog : public AudioStream
{
public:
	AudioOutputAnalog(void) : AudioStream(1, inputQueueArray) { begin(); }
	virtual void update(void);
	void begin(void);
	void analogReference(int ref);
	friend void dma_ch4_isr(void);
private:
	static audio_block_t *block_left_1st;
	static audio_block_t *block_left_2nd;
	static bool update_responsibility;
	audio_block_t *inputQueueArray[1];
};





class AudioPrint : public AudioStream
{
public:
	AudioPrint(const char *str) : AudioStream(1, inputQueueArray), name(str) {}
	virtual void update(void);
private:
	const char *name;
	audio_block_t *inputQueueArray[1];
};





















class AudioInputI2S : public AudioStream
{
public:
	AudioInputI2S(void) : AudioStream(0, NULL) { begin(); }
	virtual void update(void);
	void begin(void);
	friend void dma_ch1_isr(void);
protected:	
	AudioInputI2S(int dummy): AudioStream(0, NULL) {} // to be used only inside AudioInputI2Sslave !!
	static bool update_responsibility;  // TODO: implement and test this.
private:
	static audio_block_t *block_left;
	static audio_block_t *block_right;
	static uint16_t block_offset;
};


class AudioOutputI2S : public AudioStream
{
public:
	AudioOutputI2S(void) : AudioStream(2, inputQueueArray) { begin(); }
	virtual void update(void);
	void begin(void);
	friend void dma_ch0_isr(void);
	friend class AudioInputI2S;
protected:
	AudioOutputI2S(int dummy): AudioStream(2, inputQueueArray) {} // to be used only inside AudioOutputI2Sslave !!
	static void config_i2s(void);
	static audio_block_t *block_left_1st;
	static audio_block_t *block_right_1st;
	static bool update_responsibility;
private:
	static audio_block_t *block_left_2nd;
	static audio_block_t *block_right_2nd;
	static uint16_t block_left_offset;
	static uint16_t block_right_offset;
	audio_block_t *inputQueueArray[2];
};


class AudioInputI2Sslave : public AudioInputI2S
{
public:
	AudioInputI2Sslave(void) : AudioInputI2S(0) { begin(); }
	void begin(void);
	friend void dma_ch1_isr(void);
};


class AudioOutputI2Sslave : public AudioOutputI2S
{
public:
	AudioOutputI2Sslave(void) : AudioOutputI2S(0) { begin(); } ;
	void begin(void);
	friend class AudioInputI2Sslave;
	friend void dma_ch0_isr(void);
protected:
	static void config_i2s(void);
};





class AudioInputAnalog : public AudioStream
{
public:
        AudioInputAnalog(unsigned int pin) : AudioStream(0, NULL) { begin(pin); }
        virtual void update(void);
        void begin(unsigned int pin);
        friend void dma_ch2_isr(void);
private:
        static audio_block_t *block_left;
        static uint16_t block_offset;
	uint16_t dc_average;
        //static bool update_responsibility;  // TODO: implement and test this.
};




















#include "SD.h"

class AudioPlaySDcardWAV : public AudioStream
{
public:
	AudioPlaySDcardWAV(void) : AudioStream(0, NULL) { begin(); }
	void begin(void);

	bool play(const char *filename);
	void stop(void);
	bool start(void);
	virtual void update(void);
private:
	File wavfile;
	bool consume(void);
	bool parse_format(void);
	uint32_t header[5];
	uint32_t data_length;		// number of bytes remaining in data section
	audio_block_t *block_left;
	audio_block_t *block_right;
	uint16_t block_offset;
	uint8_t buffer[512];
	uint16_t buffer_remaining;
	uint8_t state;
	uint8_t state_play;
	uint8_t leftover_bytes;
};


class AudioPlaySDcardRAW : public AudioStream
{
public:
	AudioPlaySDcardRAW(void) : AudioStream(0, NULL) { begin(); }
	void begin(void);
	bool play(const char *filename);
	void stop(void);
	virtual void update(void);
private:
	File rawfile;
	audio_block_t *block;
	bool playing;
	bool paused;
};



class AudioPlayMemory : public AudioStream
{
public:
	AudioPlayMemory(void) : AudioStream(0, NULL), playing(0) { }
	void play(const unsigned int *data);
	void stop(void);
	virtual void update(void);
private:
	const unsigned int *next;
	uint32_t length;
	int16_t prior;
	volatile uint8_t playing;
};










class AudioMixer4 : public AudioStream
{
public:
        AudioMixer4(void) : AudioStream(4, inputQueueArray) {
		for (int i=0; i<4; i++) multiplier[i] = 65536;
	}
        virtual void update(void);
	void gain(unsigned int channel, float gain) {
		if (channel >= 4) return;
		if (gain > 32767.0f) gain = 32767.0f;
		else if (gain < 0.0f) gain = 0.0f;
		multiplier[channel] = gain * 65536.0f; // TODO: proper roundoff?
	}
private:
	int32_t multiplier[4];
	audio_block_t *inputQueueArray[4];
};






class AudioFilterBiquad : public AudioStream
{
public:
	AudioFilterBiquad(int *parameters)
	   : AudioStream(1, inputQueueArray), definition(parameters) { }
	virtual void update(void);
private:
	int *definition;
	audio_block_t *inputQueueArray[1];
};







class AudioAnalyzeToneDetect : public AudioStream
{
public:
	AudioAnalyzeToneDetect(void)
	  : AudioStream(1, inputQueueArray), thresh(6554), enabled(false) { }
	void frequency(float freq, uint16_t cycles=10) {
		set_params((int32_t)(cos((double)freq
		  * (2.0 * 3.14159265358979323846 / AUDIO_SAMPLE_RATE_EXACT))
		  * (double)2147483647.999), cycles,
		  (float)AUDIO_SAMPLE_RATE_EXACT / freq * (float)cycles + 0.5f);
	}
	void set_params(int32_t coef, uint16_t cycles, uint16_t len);
	bool available(void) {
		__disable_irq();
		bool flag = new_output;
		if (flag) new_output = false;
		__enable_irq();
		return flag;
	}
	float read(void);
	void threshold(float level) {
		if (level < 0.01f) thresh = 655;
		else if (level > 0.99f) thresh = 64881;
		else thresh = level * 65536.0f + 0.5f;
	}
	operator bool();  // true if at or above threshold, false if below
	virtual void update(void);
private:
	int32_t coefficient;	// Goertzel algorithm coefficient
	int32_t s1, s2;		// Goertzel algorithm state
	int32_t out1, out2;	// Goertzel algorithm state output
	uint16_t length;	// number of samples to analyze
	uint16_t count;		// how many left to analyze
	uint16_t ncycles;	// number of waveform cycles to seek
	uint16_t thresh;	// threshold, 655 to 64881 (1% to 99%)
	bool enabled;
	volatile bool new_output;
	audio_block_t *inputQueueArray[1];
};







// TODO: more audio processing objects....
//  sine wave with frequency modulation (phase)
//  waveforms with bandwidth limited tables for synth
//  envelope: attack-decay-sustain-release, maybe other more complex?
//  MP3 decoding - it is possible with optimized code?
//  other decompression, ADPCM, Vorbis, Speex, etc?




// A base class for all Codecs, DACs and ADCs, so at least the
// most basic functionality is consistent.

#define AUDIO_INPUT_LINEIN  0
#define AUDIO_INPUT_MIC     1

class AudioControl
{
public:
	virtual bool enable(void) = 0;
	virtual bool disable(void) = 0;
	virtual bool volume(float volume) = 0;      // volume 0.0 to 100.0
	virtual bool inputLevel(float volume) = 0;  // volume 0.0 to 100.0
	virtual bool inputSelect(int n) = 0;
};



class AudioControlWM8731 : public AudioControl
{
public:
	bool enable(void);
	bool disable(void) { return false; }
	bool volume(float n) { return volumeInteger(n * 0.8 + 47.499); }
	bool inputLevel(float n) { return false; }
	bool inputSelect(int n) { return false; }
protected:
	bool write(unsigned int reg, unsigned int val);
	bool volumeInteger(unsigned int n); // range: 0x2F to 0x7F
};

class AudioControlWM8731master : public AudioControlWM8731
{
public:
	bool enable(void);
};


class AudioControlSGTL5000 : public AudioControl
{
public:
	bool enable(void);
	bool disable(void) { return false; }
	bool volume(float n) { return volumeInteger(n * 1.29 + 0.499); }
	bool inputLevel(float n) {return false;}
	bool muteHeadphone(void) { return write(0x0024, ana_ctrl | (1<<4)); }
	bool unmuteHeadphone(void) { return write(0x0024, ana_ctrl & ~(1<<4)); }
	bool muteLineout(void) { return write(0x0024, ana_ctrl | (1<<8)); }
	bool unmuteLineout(void) { return write(0x0024, ana_ctrl & ~(1<<8)); }
	bool inputSelect(int n) {
		if (n == AUDIO_INPUT_LINEIN) {
			return write(0x0024, ana_ctrl | (1<<2));
		} else if (n == AUDIO_INPUT_MIC) {
			//return write(0x002A, 0x0172) && write(0x0024, ana_ctrl & ~(1<<2));
			return write(0x002A, 0x0173) && write(0x0024, ana_ctrl & ~(1<<2)); // +40dB
		} else {
			return false;
		}
	}
	//bool inputLinein(void) { return write(0x0024, ana_ctrl | (1<<2)); }
	//bool inputMic(void) { return write(0x002A, 0x0172) && write(0x0024, ana_ctrl & ~(1<<2)); }
protected:
	bool muted;
	bool volumeInteger(unsigned int n); // range: 0x00 to 0x80
	uint16_t ana_ctrl;



	unsigned int read(unsigned int reg);
	bool write(unsigned int reg, unsigned int val);
};
















