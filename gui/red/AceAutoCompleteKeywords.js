var AceAutoComplete  = (function() {
    var Extension=[
        { "name":"begin", 				"value":"begin();", 						  "meta": "begin" },
        { "caption":"begin", 				"snippet":"void begin()\n{\n    ${1}\n}\n", 			  "meta": "snippet" , "type":"snippet"},
        ];

    var ClassFunctions={
        "AudioSynthWaveform":[
            { "name":"begin", 			    "value":"begin(waveform);", 				  "meta": "Configure the waveform type to create." },
            { "name":"begin", 			    "value":"begin(level, frequency, waveform);", "meta": "Output a waveform, and set the amplitude and base frequency." },
            { "name":"frequency", 		    "value":"frequency(freq);", 				  "meta": "Change the base (unmodulated) frequency" },
            { "name":"amp", 		        "value":"amplitude(freq);", 				  "meta": "Change the amplitude. Set to 0 to turn the signal off." },
            { "name":"offset", 		        "value":"offset(freq);", 				      "meta": "Add a DC offset, from -1.0 to +1.0.<br>Useful for generating waveforms to <br>use as control or modulation signals." },
            { "name":"phase",               "value":"phase(angle);", 	                  "meta": "Cause the generated waveform to jump<br> to a specific point within its cycle.<br> Angle is from 0 to 360 degrees.<br> When multiple objects are configured,<br> AudioNoInterrupts() should be used to <br>guarantee all new settings take effect together." },
            { "name":"pulseWidth",          "value":"pulseWidth(amount);", 	              "meta": "Change the width (duty cycle) of the pulse." },
            { "name":"arbitraryWaveform",   "value":"arbitraryWaveform(array, maxFreq);", "meta": 'Configure the waveform to be used with WAVEFORM_ARBITRARY.<br> Array must be an array of 256 samples. Currently, <br>the data is used without any filtering,<br> which can cause aliasing with frequencies above 172 Hz. <br>For higher frequency output, <br>you must bandwidth limit your waveform data. <br>Someday, "maxFreq" will be used to do this automatically.' },
            ],
        "AudioSynthWaveformModulated":[
            { "name":"begin", 			    "value":"begin(waveform);", 				  "meta": "Configure the waveform type to create." },
            { "name":"begin", 			    "value":"begin(level, frequency, waveform);", "meta": "Output a waveform, and set the amplitude and base frequency." },
            { "name":"frequency", 		    "value":"frequency(freq);", 				  "meta": "Change the base (unmodulated) frequency" },
            { "name":"amp", 		        "value":"amplitude(freq);", 				  "meta": "Change the amplitude. Set to 0 to turn the signal off." },
            { "name":"offset", 		        "value":"offset(freq);", 				      "meta": "Add a DC offset, from -1.0 to +1.0.<br> Useful for generating waveforms to use as control or modulation signals." },
            { "name":"frequencyModulation", "value":"frequencyModulation(octaves);", 	  "meta": "Configure for frequency modulation mode <br>(the default) where the input signal will adjust the frequency by a specific<br>number of octaves (the default is 8 octaves).<br>If the -1.0 to +1.0 signal represents a ±10 volt range and you wish to have control at 1 volt/octave,<br>then configure for 10 octaves range.<br>The maximum modulation sensitivity is 12 octaves." },
            { "name":"phaseModulation",     "value":"phaseModulation(degrees);", 	      "meta": "Configure for phase modulation mode where the input signal will adjust the waveform phase angle a specific number of degrees.<br>180.0 allows a full scale ±1.0 signal to span 1 full cycle of the waveform.<br> Maximum modulation sensitivity is 9000 degrees (±25 cycles)." },
            { "name":"arbitraryWaveform",   "value":"arbitraryWaveform(array, maxFreq);", "meta": 'Configure the waveform to be used with WAVEFORM_ARBITRARY.<br>Array must be an array of 256 samples.<br>Currently, the data is used without any filtering, which can cause aliasing with frequencies above 172 Hz.<br>For higher frequency output, you must bandwidth limit your waveform data.<br>Someday, "maxFreq" will be used to do this automatically.' },
            ]
    };

    return {
        ClassFunctions:ClassFunctions,
        Extension:Extension
    }
})();