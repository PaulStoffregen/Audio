/* public domain
 * vim: set ts=4:
 */

RED.storage = (function() {
	function update() {

		//RED.nodes.addClassTabsToPalette(); //Jannik
		//RED.nodes.refreshClassNodes(); //Jannik
		
		// TOOD: use setTimeout to limit the rate of changes?
		// (Jannik say that is not needed because it's better to save often, not to loose any changes)
		// it's only needed when we move objects with keyboard, 
		// but then the save timeOut should be at keyboard move function not here.
		// TODO: save when using keyboard to move nodes.
		
		if (localStorage)
		{
			var nns = RED.nodes.createCompleteNodeSet();
			localStorage.setItem("audio_library_guitool",JSON.stringify(nns));
			console.log("localStorage write");
		}
	}
	function allStorage() {

		var archive = [],
			keys = Object.keys(localStorage),
			i = 0, key;
	
		for (; key = keys[i]; i++) {
			archive.push( key + '=' + localStorage.getItem(key));
		}
	
		return archive;
	}
	function load() {
		const t0 = performance.now();
		if (localStorage) {
			//console.warn(allStorage());
			var json_string = localStorage.getItem("audio_library_guitool");
			console.log("localStorage read: " );//+ json_string);

			if (json_string && (json_string.trim().length != 0))
			{
				var jsonObj = JSON.parse(json_string);

				if (jsonObj.workspaces) // new version
				{
					RED.nodes.importWorkspaces(jsonObj.workspaces);
				}
				else
					RED.nodes.import(json_string, false);
			}
			else
				RED.nodes.createNewDefaultWorkspace();
		}
		const t1 = performance.now();
		console.log('storage-load took: ' + (t1-t0) +' milliseconds.');
	}
	function loadContents(json_string) {
		console.log("loadContents:" +json_string);
		localStorage.setItem("audio_library_guitool", json_string);
		window.location.reload();
		
				
	}
	function clear() {
		// TOOD: use setTimeout to limit the rate of changes?
		if (localStorage)
		{
			localStorage.removeItem("audio_library_guitool");
			//console.log("localStorage write");
		}
	}
	return {
		update: update,
		load: load,
		loadContents:loadContents, 
		clear: clear
	}
})();
