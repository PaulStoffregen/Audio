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
  	var source = context.createBufferSource();
  	source.buffer = buffer;
  	source.loop = false;
  	source.connect(context.destination);
  	source.start(0); 
	});
}

function generateOutputFile(fileContents) {
  var textFileURL = null;
  var blob = new Blob([fileContents], {type: 'text/plain'});
  textFileURL = window.URL.createObjectURL(blob);
  return textFileURL;
}

var outputFileHolder = document.getElementById('outputFileHolder');
var downloadLink = document.createElement('a');
downloadLink.setAttribute('download', 'test.txt');
downloadLink.href = generateOutputFile("this is a test");
downloadLink.innerHTML = 'download link';
outputFileHolder.appendChild(downloadLink);