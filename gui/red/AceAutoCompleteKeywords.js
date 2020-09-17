var AceAutoComplete  = (function() {
    var Extension=[
        { "name":"begin", 				        "value":"begin();", 						  "meta": "begin" },
        { "caption":"begin", 				    "snippet":"void begin()\n{\n    ${1}\n}\n", 			  "meta": "snippet" , "type":"snippet"},
        { "name":"WAVEFORM_SINE",               "value":"WAVEFORM_SINE", "meta":"sinus waveform"},
        { "name":"WAVEFORM_SAWTOOTH",           "value":"WAVEFORM_SAWTOOTH", "meta":"saw waveform"},
        { "name":"WAVEFORM_SAWTOOTH_REVERSE",   "value":"WAVEFORM_SAWTOOTH_REVERSE", "meta":"reversed saw waveform"},
        { "name":"WAVEFORM_SQUARE",             "value":"WAVEFORM_SQUARE", "meta":"square waveform"},
        { "name":"WAVEFORM_TRIANGLE",           "value":"WAVEFORM_TRIANGLE", "meta":"triangle waveform"},
        { "name":"WAVEFORM_TRIANGLE_VARIABLE",  "value":"WAVEFORM_TRIANGLE_VARIABLE", "meta":"variable triangle waveform"},
        { "name":"WAVEFORM_ARBITRARY",          "value":"WAVEFORM_ARBITRARY", "meta":"arbitrary waveform"},
        { "name":"WAVEFORM_PULSE",              "value":"WAVEFORM_PULSE", "meta":"pulse waveform"},
        { "name":"WAVEFORM_SAMPLE_HOLD",        "value":"WAVEFORM_SAMPLE_HOLD", "meta":"sample hold waveform"},
        ];

    var ClassFunctions={
        "AudioSynthWaveform":[
            { "name":"begin", 			    "value":"begin(waveform);", 				  "meta": "Configure the waveform type to create.<br><img src='img/waveformsmod.png'>" },
            { "name":"begin", 			    "value":"begin(level, frequency, waveform);", "meta": "Output a waveform, and set the amplitude and base frequency." },
            { "name":"frequency", 		    "value":"frequency(freq);", 				  "meta": "Change the base (unmodulated) frequency" },
            { "name":"amplitude", 		    "value":"amplitude(freq);", 				  "meta": "Change the amplitude. Set to 0 to turn the signal off." },
            { "name":"offset", 		        "value":"offset(freq);", 				      "meta": "Add a DC offset, from -1.0 to +1.0. Useful for generating waveforms to use as control or modulation signals." },
            { "name":"phase",               "value":"phase(angle);", 	                  "meta": "Cause the generated waveform to jump to a specific point within its cycle. Angle is from 0 to 360 degrees. When multiple objects are configured, AudioNoInterrupts() should be used to guarantee all new settings take effect together." },
            { "name":"pulseWidth",          "value":"pulseWidth(amount);", 	              "meta": "Change the width (duty cycle) of the pulse." },
            { "name":"arbitraryWaveform",   "value":"arbitraryWaveform(array, maxFreq);", "meta": 'Configure the waveform to be used with WAVEFORM_ARBITRARY. Array must be an array of 256 samples. Currently, the data is used without any filtering, which can cause aliasing with frequencies above 172 Hz. For higher frequency output, you must bandwidth limit your waveform data. Someday, "maxFreq" will be used to do this automatically.' },
            ],
        "AudioSynthWaveformModulated":[
            { "name":"begin", 			    "value":"begin(waveform);", 				  "meta": "Configure the waveform type to create.<img src='img/waveformsmod.png'>" },
            { "name":"begin", 			    "value":"begin(level, frequency, waveform);", "meta": "Output a waveform, and set the amplitude and base frequency." },
            { "name":"frequency", 		    "value":"frequency(freq);", 				  "meta": "Change the base (unmodulated) frequency" },
            { "name":"amplitude", 		    "value":"amplitude(freq);", 				  "meta": "Change the amplitude. Set to 0 to turn the signal off." },
            { "name":"offset", 		        "value":"offset(freq);", 				      "meta": "Add a DC offset, from -1.0 to +1.0. Useful for generating waveforms to use as control or modulation signals." },
            { "name":"frequencyModulation", "value":"frequencyModulation(octaves);", 	  "meta": "Configure for frequency modulation mode (the default) where the input signal will adjust the frequency by a specificnumber of octaves (the default is 8 octaves). If the -1.0 to +1.0 signal represents a ±10 volt range and you wish to have control at 1 volt/octave, then configure for 10 octaves range. The maximum modulation sensitivity is 12 octaves." },
            { "name":"phaseModulation",     "value":"phaseModulation(degrees);", 	      "meta": "Configure for phase modulation mode where the input signal will adjust the waveform phase angle a specific number of degrees.180. 0 allows a full scale ±1.0 signal to span 1 full cycle of the waveform. Maximum modulation sensitivity is 9000 degrees (±25 cycles)." },
            { "name":"arbitraryWaveform",   "value":"arbitraryWaveform(array, maxFreq);", "meta": 'Configure the waveform to be used with WAVEFORM_ARBITRARY. Array must be an array of 256 samples. Currently, the data is used without any filtering, which can cause aliasing with frequencies above 172 Hz. For higher frequency output, you must bandwidth limit your waveform data. Someday, "maxFreq" will be used to do this automatically.' },
            ]
    };

    return {
        ClassFunctions:ClassFunctions,
        Extension:Extension
    }
})();









