var audioFileChooser = document.getElementById('audioFileChooser');

audioFileChooser.addEventListener('change', readFile);

function readFile() {
  // TODO: deal with multiple files
  var fileReader = new FileReader();
  fileReader.readAsArrayBuffer(audioFileChooser.files[0]);
  fileReader.addEventListener('load', function(ev) {
    processFile(ev.target.result);
  });
}

function processFile(file) {
  var context = new OfflineAudioContext(1,10*44100,44100);
	context.decodeAudioData(file, function(buffer) {
  	var monoData = buffer.getChannelData(0); // start with mono, do stereo later
    var outputData = [];
    for(var i=0;i<monoData.length;i+=2) {
      var a = floatToUnsignedInt16(monoData[i]);
      var b = floatToUnsignedInt16(monoData[i+1]);
      outputData.push((65536*b + a).toString(16));
    }
    var padLength = padding(outputData.length, 128);
    for(var i=0;i<padLength;i++) {
      outputData.push((0).toString(16));
    }
    console.log(outputData);
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

function generateCPPFile(fileName, audioData) {
  var out = "";
  out += '// Audio data converted from audio file by wav2sketch_js\n\n';
  out += '#include "AudioSample' + fileName + '.h"\n\n';
  out += 'const unsigned int AudioSample' + fileName + '[' + audioData.length + '] = {';
  out += audioData.join(',') + ',};';
  return out;
}

var outputFileHolder = document.getElementById('outputFileHolder');
var downloadLink = document.createElement('a');
downloadLink.setAttribute('download', 'test.txt');
downloadLink.href = generateOutputFile(generateCPPFile('test.wav',[0,1,2,3]));
downloadLink.innerHTML = 'download link';
outputFileHolder.appendChild(downloadLink);