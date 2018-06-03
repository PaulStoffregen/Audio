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
  var context = new window.AudioContext();
	context.decodeAudioData(file, function(buffer) {
  	var monoData = buffer.getChannelData(0); // start with mono, do stereo later
    var thisLength = monoData.length;
    
    // AudioPlayMemory requires padding to 2.9 ms boundary (128 samples @ 44100)
    var padLength = padding(thisLength, 128);
    var arraylen = ((thisLength + padLength) * 2 + 3) / 4 + 1;
    console.log(10241, thisLength, padLength, arraylen);
    console.log(monoData);
	});
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