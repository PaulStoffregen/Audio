synth_waveform 140404
	This synthesizes a waveform of the specified type with a given amplitude
	and frequency. There are currently four types of waveform:
	#define TONE_TYPE_SINE     0
	#define TONE_TYPE_SAWTOOTH 1
	#define TONE_TYPE_SQUARE   2
	#define TONE_TYPE_TRIANGLE 3
	Sine wave generation uses a lookup table and linear interpolation.
	The other three waveforms are generated directly without using table lookup.
	
boolean begin(float t_amp,float t_freq,short t_type)
	This starts generation of a waveform of given type, amplitude and frequency
	Example: begin(0.8,440.0,TONE_TYPE_SINE)
	
void set_ramp_length(int16_t r_length)
	When a tone starts, or ends, playing it can generate an audible "thump" which can
	be very distracting, especially when playing musical notes. This function specifies
	a "ramp" length (in number of samples) and the beginning of the generated waveform
	will be ramped up in volume from zero to t_amp over the course of r_length samples.
	When the tone is switched off, by changing its volume to zero, instead of ending
	abruptly it will be ramped down to zero over the next r_length samples. 
	For example, if r_length is 44, the beginning and end of the wave will have a ramp
	of approximately one millisecond.
	
void frequency(float t_freq)
	Changes the frequency of the wave to the specified t_freq. This is done in a phase-
	continuous manner which should allow generation of audio frequency shift keying and
	other effects requiring a changing frequency.
	If the frequency is set to zero sample generation is stopped.
	
void amplitude(float n)
	Changes the amplitude to 'n'. If 'n' is zero the wave is turned off and any further
	audio output will be zero.


  