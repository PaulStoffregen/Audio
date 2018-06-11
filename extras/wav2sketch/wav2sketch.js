/*
  TODO
  u-law encoding
  browser feature check (offlineaudiocontext might still be a bit niche)
*/

var audioFileChooser = document.getElementById('audioFileChooser');

audioFileChooser.addEventListener('change', readFile);

function readFile() {
  for(var i = 0; i < audioFileChooser.files.length; i++) {
    var fileReader = new FileReader();
    var sampleRateChoice = document.getElementById('sampleRate').value;
    if(sampleRateChoice == "auto") sampleRateChoice = null;
    else sampleRateChoice = parseInt(sampleRateChoice);
    fileReader.readAsArrayBuffer(audioFileChooser.files[i]);
    fileReader.addEventListener('load', function(fileName, ev) {
      processFile(ev.target.result, fileName, sampleRateChoice);
    }.bind(null, audioFileChooser.files[i].name));
  }
}

function processFile(file, fileName, sampleRateChoice) {
  // determine sample rate
  // ideas borrowed from https://github.com/ffdead/wav.js
  var sampleRate = 0;
  if(!sampleRateChoice) {
    try {
      var sampleRateBytes = new Uint8Array(file, 24, 4);
      for(var i = 0; i < sampleRateBytes.length; i ++) {
        sampleRate |= sampleRateBytes[i] << (i*8);
      }
    } catch(err) {
      console.log('problem reading sample rate');
    }
    if([44100, 22050, 11025].indexOf(sampleRate) == -1) {
      sampleRate = 44100;
    }
  } else {
    sampleRate = sampleRateChoice;
  }

  var context = new OfflineAudioContext(1, 100*sampleRate, sampleRate); // arbitrary 100 seconds max length for now, nothing that long would fit on a Teensy anyway
	context.decodeAudioData(file, function(buffer) {
  	var monoData = [];
    if(buffer.numberOfChannels == 1) {
      monoData = buffer.getChannelData(0);
    } else if(buffer.numberOfChannels == 2) {
      var leftData = buffer.getChannelData(0);
      var rightData = buffer.getChannelData(1);
      for(var i=0;i<buffer.length;i++) {
        monoData[i] = (leftData[i] + rightData[i]) / 2;
      }
    } else {
      alert("ONLY MONO AND STEREO FILES ARE SUPPORTED");
      // NB - would be trivial to add support for n channels, given that everything ends up mono anyway
    }
    var padLength;
    var compressionCode = '0';
    var sampleRateCode;
    if(true) compressionCode = '8'; // add u-law support here later
    if(sampleRate == 44100) {
      padLength = padding(monoData.length, 128);
      sampleRateCode = '1';
    } else if(sampleRate == 22050) {
      padLength = padding(monoData.length, 64);
      sampleRateCode = '2';
    } else if(sampleRate == 11025) {
      padLength = padding(monoData.length, 32);
      sampleRateCode = '3';
    }

    var ulawOut = [];
    for(var i = 0; i < monoData.length; i ++) {
      ulawOut.push(ulaw_encode(toInteger(monoData[i]*0x7fff)).toString(16));
    }
    window.ulawOut = ulawOut;
    var outputData = createWords(monoData, padLength);

    var statusInt = (monoData.length).toString(16);
    while(statusInt.length < 6) statusInt = '0' + statusInt;
    if(monoData.length>0xFFFFFF) alert("DATA TOO LONG");
    statusInt = '0x' + compressionCode + sampleRateCode + statusInt;
    outputData.unshift(statusInt);

    var outputFileHolder = document.getElementById('outputFileHolder');
    var downloadLink1 = document.createElement('a');
    var downloadLink2 = document.createElement('a');
    var formattedName = fileName.split('.')[0];
    formattedName = formattedName.charAt(0).toUpperCase() + formattedName.slice(1).toLowerCase();
    downloadLink1.href = generateOutputFile(generateCPPFile(fileName, formattedName, outputData, sampleRate));
    downloadLink1.setAttribute('download', 'AudioSample' + formattedName + '.cpp');
    downloadLink1.innerHTML = 'Download AudioSample' + formattedName + '.cpp';
    downloadLink2.href = generateOutputFile(generateHeaderFile(formattedName, outputData));
    downloadLink2.setAttribute('download', 'AudioSample' + formattedName + '.h');
    downloadLink2.innerHTML = 'Download AudioSample' + formattedName + '.h';
    outputFileHolder.appendChild(downloadLink1);
    outputFileHolder.appendChild(document.createElement('br'));
    outputFileHolder.appendChild(downloadLink2);
    outputFileHolder.appendChild(document.createElement('br'));
	});
}

function ulaw_encode(audio)
{
	var mag, neg; // both uint32

	// http://en.wikipedia.org/wiki/G.711
	if (audio >= 0) {
		mag = audio;
		neg = 0;
	} else {
		mag = audio * -1;
		neg = 0x80;
	}
	mag += 128;
	if (mag > 0x7FFF) mag = 0x7FFF;
	if (mag >= 0x4000) return neg | 0x70 | ((mag >> 10) & 0x0F);  // 01wx yz00 0000 0000
	if (mag >= 0x2000) return neg | 0x60 | ((mag >> 9) & 0x0F);   // 001w xyz0 0000 0000
	if (mag >= 0x1000) return neg | 0x50 | ((mag >> 8) & 0x0F);   // 0001 wxyz 0000 0000
	if (mag >= 0x0800) return neg | 0x40 | ((mag >> 7) & 0x0F);   // 0000 1wxy z000 0000
	if (mag >= 0x0400) return neg | 0x30 | ((mag >> 6) & 0x0F);   // 0000 01wx yz00 0000
	if (mag >= 0x0200) return neg | 0x20 | ((mag >> 5) & 0x0F);   // 0000 001w xyz0 0000
	if (mag >= 0x0100) return neg | 0x10 | ((mag >> 4) & 0x0F);   // 0000 0001 wxyz 0000
	                   return neg | 0x00 | ((mag >> 3) & 0x0F);   // 0000 0000 1wxy z000
}

function createWords(audioData, padLength) {
  var totalLength = audioData.length + padLength;
  var outputData = [];
  for(var i = 0; i < totalLength; i += 2) {
    var a = toUint16(i<audioData.length?audioData[i]*0x7fff:0x0000);
    var b = toUint16(i+1<audioData.length?audioData[i+1]*0x7fff:0x0000);
    var out = (65536*b + a).toString(16);
    while(out.length < 8) out = '0' + out;
    out = '0x' + out;
    outputData.push(out);
  }
  return outputData;
}

// http://2ality.com/2012/02/js-integers.html
function toInteger(x) {
  x = Number(x);
  return Math.round(x);
  //return x < 0 ? Math.ceil(x) : Math.floor(x);
}

function modulo(a, b) {
  return a - Math.floor(a/b)*b;
}

function toUint16(x) {
  return modulo(toInteger(x), Math.pow(2, 16));
}

function toUint8(x) {
  return modulo(toInteger(x), Math.pow(2, 8));
}

// compute the extra padding needed
function padding(sampleLength, block) {
	var extra = sampleLength % block;
	if (extra == 0) return 0;
	return block - extra;
}

function generateOutputFile(fileContents) {
  var textFileURL = null;
  var blob = new Blob([fileContents], {type: 'text/plain'});
  textFileURL = window.URL.createObjectURL(blob);
  return textFileURL;
}

function formatAudioData(audioData) {
  var outputString = '';
  for(var i = 0; i < audioData.length; i ++) {
    if(i%8==0 && i>0) outputString += '\n';
    outputString += audioData[i] + ',';
  }
  return outputString;
}

function generateCPPFile(fileName, formattedName, audioData, sampleRate, encodingType) {
  var out = "";
  out += '// Audio data converted from audio file by wav2sketch_js\n\n';
  out += '#include "AudioSample' + formattedName + '.h"\n\n';
  out += '// Converted from ' + fileName + ', using ' + sampleRate + ' Hz, 16 bit PCM encoding\n';
  out += 'const unsigned int AudioSample' + formattedName + '[' + audioData.length + '] = {\n';
  out += formatAudioData(audioData) + '\n};';
  return out;
}

function generateHeaderFile(formattedName, audioData) {
  var out = "";
  out += '// Audio data converted from audio file by wav2sketch_js\n\n';
  out += 'extern const unsigned int AudioSample' + formattedName + '[' + audioData.length + '];';
  return out;
}
