/** Modified from original Node-Red source, for audio system visualization
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
RED.sidebar.info = (function() {
	
	var content = document.createElement("div");
	content.id = "tab-info";
	content.style.paddingTop = "4px";
	content.style.paddingLeft = "4px";
	content.style.paddingRight = "4px";

	RED.sidebar.addTab("info",content);
	$("#tab-info").html("<h3>Welcome</h3><p>The Audio System Design Tool lets you easily draw a system to process 16 bit, 44.1 kHz streaming audio while your Arduino sketch also runs.</p><p>Export will generate code to copy into the Arduino editor, to implement your system.</p><p>Most objects provide simple functions you can call from setup() or loop() to control your audio project!</p><h3>Offline Use</h3><p>This tool does not use a server.  A stand-alone copy is provided with the Teensy Audio Library, in the gui folder.</p><h3>Credits</h3><p>Special thanks to Nicholas O'Leary, Dave Conway-Jones and IBM.</p><p>Without their work on the open source <a href=\"http://nodered.org/\" target=\"_blank\">Node-RED</a> project, this graphical design tool would not have been possible!</p>");

	function jsonFilter(key,value) {
		if (key === "") {
			return value;
		}
		var t = typeof value;
		if ($.isArray(value)) {
			return "[array:"+value.length+"]";
		} else if (t === "object") {
			return "[object]"
		} else if (t === "string") {
			if (value.length > 30) {
				return value.substring(0,30)+" ...";
			}
		}
		return value;
	}
	
	function refresh(node) {
		var table = '<table class="node-info"><tbody>';

		table += "<tr><td>Type</td><td>&nbsp;"+node.type+"</td></tr>";
		table += "<tr><td>ID</td><td>&nbsp;"+node.id+"</td></tr>";
		table += '<tr class="blank"><td colspan="2">&nbsp;Properties</td></tr>';
		for (var n in node._def.defaults) {
			if (node._def.defaults.hasOwnProperty(n)) {
				var val = node[n]||"";
				var type = typeof val;
				if (type === "string") {
					if (val.length > 30) { 
						val = val.substring(0,30)+" ...";
					}
					val = val.replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;");
				} else if (type === "number") {
					val = val.toString();
				} else if ($.isArray(val)) {
					val = "[<br/>";
					for (var i=0;i<Math.min(node[n].length,10);i++) {
						var vv = JSON.stringify(node[n][i],jsonFilter," ").replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;");
						val += "&nbsp;"+i+": "+vv+"<br/>";
					}
					if (node[n].length > 10) {
						val += "&nbsp;... "+node[n].length+" items<br/>";
					}
					val += "]";
				} else {
					val = JSON.stringify(val,jsonFilter," ");
					val = val.replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;");
				}
				
				table += "<tr><td>&nbsp;"+n+"</td><td>"+val+"</td></tr>";
			}
		}
		table += "</tbody></table><br/>";
		this.setHelpContent(table, node.type);
	}

	function setHelpContent(prefix, key) {
		// server test switched off - test purposes only
		var patt = new RegExp(/^[http|https]/);
		var server = false && patt.test(location.protocol);


		prefix = prefix == "" ? "<h3>" + key + "</h3>" : prefix;
		if (!server) {
			data = $("script[data-help-name|='" + key + "']").html();
			$("#tab-info").html(prefix + '<div class="node-help">' + data + '</div>');
		} else {
			$.get( "resources/help/" + key + ".html", function( data ) {
				$("#tab-info").html(prefix + '<h2>' + key + '</h2><div class="node-help">' + data + '</div>');
			}).fail(function () {
				$("#tab-info").html(prefix);
			});
		}
	}
	
	return {
		refresh:refresh,
		clear: function() {
			$("#tab-info").html("");
		},
		setHelpContent: setHelpContent
	}
})();
