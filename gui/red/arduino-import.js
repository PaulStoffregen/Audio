/** Added in addition to original Node-Red source, for audio system visualization
 * this file is intended to work as an interface between Node-Red flow and Arduino
 * cppToJSON gets a seperate file to make it easier to find it.
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
RED.arduino.import = (function() {
    /**
	 * Parses the input string which contains copied code from the Arduino IDE, scans the
	 * nodes and connections and forms them into a JSON representation which will be
	 * returned as string.
	 *
	 * So the result may directly imported in the localStorage or the import dialog.
	 */
	function cppToJSON(newNodesStr) {

		var newNodes = [];
		var cables = [];
		var words = [];

		const NODE_COMMENT	= "//";
		const NODE_AC		= "AudioConnection";

		var parseLine = function(line) {

			var parts = line.match(/^(\S+)\s(.*)/);
			if (parts == null) {
				return
			}
			parts =	parts.slice(1);
			if (parts == null || parts.length <= 1) {
				return
			}
			var type = $.trim(parts[0]);
			line = $.trim(parts[1]) + " ";

			var name = "";
			var coords = [0, 0];
			var conn = [];

			parts = line.match(/^([^;]{0,});(.*)/);
			if (parts && parts.length >= 2) {
				parts = parts.slice(1);
				if (parts && parts.length >= 1) {
					name = $.trim(parts[0]);
					coords = $.trim(parts[1]);
					parts = coords.match(/^([^\/]{0,})\/\/xy=(.*)/);
					if (parts) {
						parts = parts.slice(1);
						coords = $.trim(parts[1]).split(",");
					}
				}
			}

			if (type == NODE_AC) {
				parts = name.match(/^([^\(]*\()([^\)]*)(.*)/);
				if (parts && parts.length > 1) {
					conn = $.trim(parts[2]).split(",");
					cables.push(conn);
				}
			} else if (type == NODE_COMMENT) {
				// do nothing
			} else {
				var names = [];
				var yPos = [];
				if (name.indexOf(",") >= 0) {
					names = name.split(",");
				} else {
					names.push(name);
				}
				for (var n = 0; n < names.length; n++) {
					name = names[n].trim();
					var gap = 10;
					var def = RED.nodes.node_defs[type];
					var dW = Math.max(RED.view.defaults.width, RED.view.calculateTextWidth(name) + (def.inputs > 0 ? 7 : 0));
					var dH = Math.max(RED.view.defaults.height,(Math.max(def.outputs, def.inputs)||0) * 15);
					var newX = parseInt(coords ? coords[0] : 0);
					var newY = parseInt(coords ? coords[1] : 0);
					//newY = newY == 0 ? lastY + (dH * n) + gap : newY;
					//lastY = Math.max(lastY, newY);
					var node = new Object({/*"order": n, */"id": name, "name": name, "type": type, "x": newX, "y": newY, "z": 0, "wires": []});
					node.z = RED.view.getWorkspace();
					
					// netter solution: create new id (this only messes everything up)
					// let import nodes take care of the new names and id:s
					/*
					if ((RED.nodes.node(node.id) !== null) && !replaceFlow) {
						node.z = RED.view.getWorkspace();
						node.id = RED.nodes.id();
						node.name = RED.nodes.getUniqueName(node); // this "crash" import when nodes with theese names allready exists
					}*/
					newNodes.push(node);
				}
			}
		};

		var findNode = function(name) {
			var len = newNodes.length;
			for (var key = 0; key < len; key++) {
				if (newNodes[key].id == name) {
					return newNodes[key];
				}
			}
		};

		var linkCables = function(cables) {
			$.each(cables, function(i, item) {
				var conn = item;
				// when there are only two entries in the array, there
				// is only one output to connect to one input, so we have
				// to extend the array with the appropriate index "0" for
				// both parst (in and out)
				if (conn.length == 2) {
					conn[2] = conn[1];
					conn[1] = conn[3] = 0;
				}
				// now we assign the outputs (marked by the "idx" of the array)
				// to the inputs describend by text
				var currNode = findNode($.trim(conn[0]));
				var idx = parseInt($.trim(conn[1]));
				if (currNode) {
					if ($.trim(conn[2]) != "" && $.trim(conn[3]) != "") {
						var wire = $.trim(conn[2]) + ":" + $.trim(conn[3]);
						var tmp = currNode.wires[idx] ? currNode.wires[idx] : [];
						tmp.push(wire);
						currNode.wires[idx] = tmp;
					}
				}
			});
		};

		var traverseLines = function(raw) {
			var lines = raw.split("\n");
			for (var i = 0; i < lines.length; i++) {
				var line = lines[i].trim();
				if (line.startsWith("//"))
					line = line.substring(2).trim();

				// we reached the setup or loop part ...
				var pattSu = new RegExp(/\s*void\s*setup\s*\(\s*\).*/);
				var pattLo = new RegExp(/\s*void\s*loop\s*\(\s*\).*/);
				if (pattSu.test(line) || pattLo.test(line)) {
					break;
				}

				// we need at least two parts to examine
				var parts = line.match(/^(\S+)\s(.*)/);
				if (parts == null || parts.length == 1) {
					continue;
				}

				// ... and it has to end with an semikolon ...
				var pattSe = new RegExp(/.*;.*$/);
				var pattCoord = new RegExp(/.*\/\/xy=\d+,\d+$/);
				if (pattSe.test(line) || pattCoord.test(line)) {
					var word = parts[1].trim();
					if (words.indexOf(word) >= 0) {
						parseLine(line);
					}
				}
			}
		};

/*
		var readCode = function() {

			var fileImport = $("#importInput")[0];
			var regex = /^([a-zA-Z0-9\s_\\.\-:])+(.ino|.txt)$/;

			if (regex.test(fileImport.value.toLowerCase())) {
				if (typeof (FileReader) != "undefined") {
					var reader = new FileReader();
					$(reader).on("load", function (e) {
					});
					reader.readAsText(fileImport.files[0]);
				} else {
					alert("This browser does not support HTML5.");
				}
			} else {
				alert("Please upload a valid INO or text file.");
			}
		};
 */
		function startImport() {
			words = Array(NODE_AC);
			$.each(RED.nodes.node_defs, function (key, obj) {
				words.push(key);
				console.log(key);
			});
			traverseLines(newNodesStr);
			linkCables(cables);
		}

		startImport();

		return {
			count: newNodes.length,
			data: newNodes.length > 0 ? JSON.stringify(newNodes) : ""
		};
    }
    return {
        cppToJSON:cppToJSON,
	};
})();