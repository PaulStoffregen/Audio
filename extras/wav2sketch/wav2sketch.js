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