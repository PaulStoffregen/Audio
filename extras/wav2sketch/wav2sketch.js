/*
  TODO
  non-44100 sample rates
  u-law encoding
*/

var audioFileChooser = document.getElementById('audioFileChooser');

audioFileChooser.addEventListener('change', readFile);

function readFile() {
  for(var i = 0; i < audioFileChooser.files.length; i++) {
    var fileReader = new FileReader();
    fileReader.readAsArrayBuffer(audioFileChooser.files[i]);
    fileReader.addEventListener('load', function(fileName, ev) {
      processFile(ev.target.result, fileName);
    }.bind(null, audioFileChooser.files[i].name));
  }
}

function processFile(file, fileName) {
  var context = new OfflineAudioContext(1,10*44100,44100);
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
    }
    var outputData = [];
    for(var i=0;i<monoData.length;i+=2) {
      var a = floatToUnsignedInt16(monoData[i]);
      var b = floatToUnsignedInt16(monoData[i+1]);
      var out = (65536*b + a).toString(16);
      while(out.length < 8) out = '0' + out;
      out = '0x' + out;
      outputData.push(out);
    }
    var padLength = padding(outputData.length, 128);

    var statusInt = (outputData.length*2).toString(16);
    while(statusInt.length < 6) statusInt = '0' + statusInt;
    if(outputData.length*2>0xFFFFFF) alert("DATA TOO LONG");
    statusInt = '0x81' + statusInt;
    outputData.unshift(statusInt);

    for(var i=0;i<padLength;i++) {
      outputData.push('0x00000000');
    }

    var outputFileHolder = document.getElementById('outputFileHolder');
    var downloadLink1 = document.createElement('a');
    var downloadLink2 = document.createElement('a');
    var formattedName = fileName.split('.')[0];
    formattedName = formattedName.charAt(0).toUpperCase() + formattedName.slice(1).toLowerCase();
    downloadLink1.href = generateOutputFile(generateCPPFile(fileName, formattedName, outputData));
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

function floatToUnsignedInt16(f) {
  f = Math.max(-1, Math.min(1, f));
  var uint;
  // horrible hacks going on here - basically i don't understand bitwise operators
  if(f>=0) uint = Math.round(f * 32767);
  else uint = Math.round((f + 1) * 32767 + 32768);
  return uint;
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
    if(i%8==0 && i>0) outputString += '\n'
    outputString += audioData[i] + ',';
  }
  return outputString;
}

function generateCPPFile(fileName, formattedName, audioData) {
  var formattedName = fileName.split('.')[0];
  formattedName = formattedName.charAt(0).toUpperCase() + formattedName.slice(1).toLowerCase();
  var out = "";
  out += '// Audio data converted from audio file by wav2sketch_js\n\n';
  out += '#include "AudioSample' + formattedName + '.h"\n\n';
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
