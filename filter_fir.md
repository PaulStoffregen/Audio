filter_fir 140418
	Filters the audio stream using FIR coefficients supplied by the user.
	The ARM	library has two q15 functions which perform an FIR filter. One uses
	a 64-bit accumulator (arm_fir_q15) and the other uses a 32-bit accumulator
	(arm_fir_fast_q15). When instantiating a filter a boolean argument specifies
	which version to use. Specifying USE_FAST_FIR (defined as boolean true) uses
	the fast code otherwise the slower code is used. For example:
	AudioFilterFIR       myFilterL(USE_FAST_FIR);

void begin(short *cp,int n_coeffs)
	Starts the filter using coefficients at *cp and the number of coefficients
	is n_coeffs. The special value FIR_PASSTHRU can be used in place of the
	address of the coefficient array, in which case the function will just pass
	audio samples through without filtering.

void stop(void)
	Stops the filter.

