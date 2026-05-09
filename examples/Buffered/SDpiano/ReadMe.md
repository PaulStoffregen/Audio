# SD-based piano sample player
An example which uses a public domain sample set to create an 8-note polyphonic 3-layer 88-note stereo piano, playing from a mix of PSRAM and SD card.

Based on public domain samples from University of Iowa Musical Instrument Samples, http://theremin.music.uiowa.edu/MIS.html. Created by Lawrence Fritts, Director of the Electronic Music Studios and Associate Professor of Composition at the University of Iowa. These have been edited to produce https://www.mediafire.com/download/cll2ifw68hohtgn/bigcat_UoIMIS_Piano_samples.zip, which is what I've used for testing.

## Requirements
 - Teensy 4.1 with 
 - audio adaptor,
 - PSRAM fitted, and 
 - a reasonably fast SD card: I used a SanDisk Extreme Plus 64Gb SDXC A2 card
 - the above sample set
 - some way of sending MIDI data to the Teensy
 
## Procedure
Unzip the sample set to the root of the SD card. You should end up with a /piano folder, within which are /soft, /med and /loud folders, and finally a bunch of WAV files in those. Note some files are missing...

Use any USB type which includes Serial and MIDI to build and upload the sketch

Make sure you open the serial port (the sketch waits forever until you do)

Play the piano!

## How it works
All samples are partially pre-loaded to PSRAM, in buffers which are an exact multiple of the audio data block size. When a MIDI note on is received, playback starts immediately from PSRAM, while the SD file is cued ready to play (this can take a few milliseconds, but there's over 170ms pre-loaded). The SD playback is then started in the exact audio cycle where the pre=loaded data runs out, giving a seamless transition to SD playback. When a MIDI note off is received, a pair of envelopes is used to give a short fade-out before SD playback is stopped, thus avoiding glitches.
 