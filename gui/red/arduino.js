/** Added in addition to original Node-Red source, for audio system visualization
 * this file is intended to work as an interface between Node-Red flow and Arduino
 * vim: set ts=4:
 * Copyright 2013 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/
RED.arduino = (function() {
	var settings = {
		useExportDialog: true,
		IOcheckAtExport: true,
		WebServerPort: 8080,
	};

	var settingsCategoryTitle = "Arduino Export/Import";

	var settingsEditorLabels = {
		useExportDialog: "Show export dialog",
		IOcheckAtExport: "IO check At Export",
		WebServerPort: "Web Server Port"
	};

    function httpPostAsync(data)
	{
		var xhr = new XMLHttpRequest();
		const url = 'http://localhost:' + settings.WebServerPort;
		xhr.open("POST", url, true);
		xhr.onloadend = function () {
			console.warn("response:" + xhr.responseText);
		  };
		xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
		xhr.send(data); 
	}
	function httpGetAsync(param)
	{
		var xmlHttp = new XMLHttpRequest();
		const url = 'http://localhost:' + settings.WebServerPort;
		xmlHttp.onreadystatechange = function() { 
			//if (xmlHttp.readyState == 4 && xmlHttp.status == 200)
				//callback(xmlHttp.responseText);
		}
		xmlHttp.onloadend = function () {
			if (param == "getJSON")
			{
				console.warn("JSON response");
				RED.storage.loadContents(xmlHttp.responseText);
			}
			console.warn("response:" + xmlHttp.responseText);
		  };
		xmlHttp.open("GET", url+"\\?cmd=" + param, true); // true for asynchronous 
		xmlHttp.send(null);
    }
    $('#btn-verify-compile').click(function() { httpGetAsync("compile"); });
	$('#btn-compile-upload').click(function() { httpGetAsync("upload"); });
	$('#btn-get-design-json').click(function() { httpGetAsync("getJSON"); });
    
    return {
		settings:settings,
		settingsCategoryTitle:settingsCategoryTitle,
		settingsEditorLabels:settingsEditorLabels,

		httpPostAsync:httpPostAsync,
		httpGetAsync:httpGetAsync,
	};
})();