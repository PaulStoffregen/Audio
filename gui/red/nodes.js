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
RED.nodes = (function() {

	var node_defs = {};
	var nodes = [];
	var configNodes = {};
	var links = []; // link structure {source:,sourcePort:,target:,targetPort:};
	var defaultWorkspace;
	var workspaces = {};
	var showWorkspaceToolbar = true;

	/**
	 * this creates a workspace object
	 */
	function createWorkspaceObject(id, label, inputs, outputs, _export) // export is a reserved word
	{
		return { type:"tab", id:id, label:label, inputs:inputs, outputs:outputs, export:_export};
	}

	var btnWworkspaceToolbar = $("#btn-workspace-toolbar");
	btnWworkspaceToolbar.toggleClass("active", showWorkspaceToolbar);

	$('#btn-moveWorkSpaceLeft').click(function() { moveWorkSpaceLeft(); });
	$('#btn-moveWorkSpaceRight').click(function() {  });
	function moveWorkSpaceLeft()
	{
		var index = getIndexOfCurrentWorkspace();
		if (index == 0) return;

		let wrapper=document.querySelector(".red-ui-tabs");
		let children=wrapper.children;
		
		wrapper.insertBefore(children[index], children[index-1]); // this works but we also must update workspaces
	}
	function getIndexOfCurrentWorkspace()
	{
		var wsId = RED.view.getWorkspace();
		var index = 0;
		console.error("workspaces.length:"+workspaces.length+"\n"+Object.getOwnPropertyNames(workspaces));
		for (i in workspaces) {
			if (workspaces.hasOwnProperty(i)) {
				if (workspaces[i].id == wsId)
				{
					console.error(Object.getOwnPropertyNames(workspaces[i]));
					console.warn("found ws index:" + index);
					return index;
				}
				else
					index++;
			}
		}
		
		return -1;
	}
	$('#btn-workspace-toolbar').click(function() {
		showWorkspaceToolbar = !showWorkspaceToolbar;
		if (showWorkspaceToolbar)
			$("#workspace-toolbar").show();
		else
			$("#workspace-toolbar").hide();
		btnWworkspaceToolbar.toggleClass("active", showWorkspaceToolbar);
		});
		
	function registerType(nt,def) {
		node_defs[nt] = def;
		// TODO: too tightly coupled into palette UI
		RED.palette.add(nt,def);
	}

	function getID() {
		var str = (1+Math.random()*4294967295).toString(16);
		console.log("getID = " + str);
		return str;
	}

	function checkID(name) {
		var i;
		for (i=0;i<nodes.length;i++) {
			//console.log("checkID, nodes[i].id = " + nodes[i].id);
			if (nodes[i].id == name) return true;
		}
/*
		for (i in workspaces) {
			if (workspaces.hasOwnProperty(i)) { }
		}
		for (i in configNodes) {
			if (configNodes.hasOwnProperty(i)) { }
		}
*/
		return false;
	}
	function checkName(name, wsId) { // jannik add
		var i;
		for (i=0;i<nodes.length;i++) {
			//console.log("checkID, nodes[i].id = " + nodes[i].id);
			if (wsId && (wsId != nodes[i].z)) continue; // skip nodes that is not in current workspace
			if (nodes[i].name == name) return true;
		}
/*
		for (i in workspaces) {
			if (workspaces.hasOwnProperty(i)) { }
		}
		for (i in configNodes) {
			if (configNodes.hasOwnProperty(i)) { }
		}
*/
		return false;
	}

	function createUniqueCppName(n, wsId) {
		//console.log("getUniqueCppName, n.type=" + n.type + ", n.name=" + n.name + ", n._def.shortName=" + n._def.shortName);
		var basename = n.name; //(n._def.shortName) ? n._def.shortName : n.type.replace(/^Analog/, "");

		//console.log("getUniqueCppName, using basename=" + basename);
		var count = 1;
		var sep = /[0-9]$/.test(basename) ? "_" : ""; // expression checks if basename ends with a number, sep = seperator
		var name;
		while (1) {
			name = basename + sep + count;
			if (!checkName(name, wsId)) break;
			count++;
		}
		//console.log("getUniqueCppName, unique name=" + name);
		return name;
	}
	
	function createUniqueCppId(n, workspaceName) {
		//console.log("getUniqueCppId, n.type=" + n.type + ", n.name=" + n.name + ", n._def.shortName=" + n._def.shortName);
		var basename = (n._def.shortName) ? n._def.shortName : n.type.replace(/^Analog/, "");
		
		if (workspaceName)
			basename = workspaceName + "_" + basename; // Jannik added

		//console.log("getUniqueCppId, using basename=" + basename);
		var count = 1;
		var sep = /[0-9]$/.test(basename) ? "_" : "";
		var name;
		while (1) {
			name = basename + sep + count;
			if (!checkID(name)) break;
			count++;
		}
		//console.log("getUniqueCppId, unique id=" + name);
		return name;
	}

	function getUniqueName(n) {
		var newName = n.name;
		if (typeof newName === "string") {
			var parts = newName.match(/(\d*)$/);
			var count = 0;
			var base = newName;
			if (parts) {
				count = isNaN(parseInt(parts[1])) ? 0 : parseInt(parts[1]);
				base = newName.replace(count, "");
			}
			while (RED.nodes.namedNode(newName) !== null) {
				count += 1;
				newName = base + count;
			}
		}
		return newName;
	}

	function getType(type) {
		return node_defs[type];
	}
	function selectNode(name) {
		if (!((document.origin == 'null') && (window.chrome))) {
			window.history.pushState(null, null, window.location.protocol + "//"
				+ window.location.host + window.location.pathname + '?info=' + name);
		}
	}
	function addNode(n) {
		if (n.type == "AudioMixerX")
		{
			if (!n.inputs)
				n.inputs = n._def.inputs;
		}
		if (n._def.category == "config") {
			configNodes[n.id] = n;
			RED.sidebar.config.refresh();
		} else {
			n.dirty = true;
			nodes.push(n);
			var updatedConfigNode = false;
			for (var d in n._def.defaults) {
				if (n._def.defaults.hasOwnProperty(d)) {
					var property = n._def.defaults[d];
					if (property.type) {
						var type = getType(property.type);
						if (type && type.category == "config") {
							var configNode = configNodes[n[d]];
							if (configNode) {
								updatedConfigNode = true;
								configNode.users.push(n);
							}
						}
					}
				}
			}
			if (updatedConfigNode) {
				RED.sidebar.config.refresh();
			}
		}
	}
	function addLink(l) {
		links.push(l);
	}
/*
	function addConfig(c) {
		configNodes[c.id] = c;
	}
*/
	function checkForIO() {
		var hasIO = false;
		RED.nodes.eachNode(function (node) {

			if ((node._def.category === "input-function") ||
				(node._def.category === "output-function")) {
				hasIO = true;
			}
		});
		return hasIO;
	}

	function getNode(id) {
		if (id in configNodes) {
			return configNodes[id];
		} else {
			for (var n in nodes) {
				if (nodes[n].id == id) {
					return nodes[n];
				}
			}
		}
		return null;
	}

	function getNodeByName(name) {
		for (var n in nodes) {
			if (nodes[n].name == name) {
				return nodes[n];
			}
		}
		return null;
	}

	function removeNode(id) {
		var removedLinks = [];
		if (id in configNodes) {
			delete configNodes[id];
			RED.sidebar.config.refresh();
		} else {
			var node = getNode(id);
			if (node) {
				nodes.splice(nodes.indexOf(node),1);
				removedLinks = links.filter(function(l) { return (l.source === node) || (l.target === node); });
				removedLinks.map(function(l) {links.splice(links.indexOf(l), 1); });
			}
			var updatedConfigNode = false;
			for (var d in node._def.defaults) {
				if (node._def.defaults.hasOwnProperty(d)) {
					var property = node._def.defaults[d];
					if (property.type) {
						var type = getType(property.type);
						if (type && type.category == "config") {
							var configNode = configNodes[node[d]];
							if (configNode) {
								updatedConfigNode = true;
								var users = configNode.users;
								users.splice(users.indexOf(node),1);
							}
						}
					}
				}
			}
			if (updatedConfigNode) {
				RED.sidebar.config.refresh();
			}
		}
		return removedLinks;
	}

	function removeLink(l) {
		var index = links.indexOf(l);
		if (index != -1) {
			links.splice(index,1);
		}
	}

	function refreshValidation() {
		for (var n=0;n<nodes.length;n++) {
			RED.editor.validateNode(nodes[n]);
		}
	}

	function addWorkspace(ws) {
		workspaces[ws.id] = ws;
	}
	function getWorkspace(id) {
		return workspaces[id];
	}
	function removeWorkspace(id) {
		delete workspaces[id];
		var removedNodes = [];
		var removedLinks = [];
		var n;
		for (n=0;n<nodes.length;n++) {
			var node = nodes[n];
			if (node.z == id) {
				removedNodes.push(node);
			}
		}
		for (n=0;n<removedNodes.length;n++) {
			var rmlinks = removeNode(removedNodes[n].id);
			removedLinks = removedLinks.concat(rmlinks);
		}
		return {nodes:removedNodes,links:removedLinks};
	}

	function getAllFlowNodes(node) {
		var visited = {};
		visited[node.id] = true;
		var nns = [node];
		var stack = [node];
		while(stack.length !== 0) {
			var n = stack.shift();
			var childLinks = links.filter(function(d) { return (d.source === n) || (d.target === n);});
			for (var i=0;i<childLinks.length;i++) {
				var child = (childLinks[i].source === n)?childLinks[i].target:childLinks[i].source;
				if (!visited[child.id]) {
					visited[child.id] = true;
					nns.push(child);
					stack.push(child);
				}
			}
		}
		return nns;
	}

	/**
	 * Converts a node to an exportable JSON Object
	 **/
	function convertNode(n, exportCreds) {
		exportCreds = exportCreds || false;
		var node = {};
		node.id = n.id;
		node.type = n.type;
		for (var d in n._def.defaults) {
			if (n._def.defaults.hasOwnProperty(d)) {
				node[d] = n[d];
			}
		}
		if(exportCreds && n.credentials) {
			node.credentials = {};
			for (var cred in n._def.credentials) {
				if (n._def.credentials.hasOwnProperty(cred)) {
					if (n.credentials[cred] != null) {
						node.credentials[cred] = n.credentials[cred];
					}
				}
			}
		}
		if (n._def.category != "config") {
			node.x = n.x;
			node.y = n.y;
			node.z = n.z;
			node.wires = [];
			for(var i=0;i<n.outputs;i++) {
				node.wires.push([]);
			}
			var wires = links.filter(function(d){return d.source === n;});
			for (var j=0;j<wires.length;j++) {
				var w = wires[j];
				node.wires[w.sourcePort].push(w.target.id + ":" + w.targetPort);
			}
		}
		return node;
	}

	/**
	 * Converts the current node selection to an exportable JSON Object
	 **/
	function createExportableNodeSet(set) {
		var nns = [];
		var exportedConfigNodes = {};
		for (var n=0;n<set.length;n++) {
			var node = set[n].n;
			var convertedNode = RED.nodes.convertNode(node);
			for (var d in node._def.defaults) {
				if (node._def.defaults[d].type && node[d] in configNodes) {
					var confNode = configNodes[node[d]];
					var exportable = getType(node._def.defaults[d].type).exportable;
					if ((exportable == null || exportable)) {
						if (!(node[d] in exportedConfigNodes)) {
							exportedConfigNodes[node[d]] = true;
							nns.unshift(RED.nodes.convertNode(confNode));
						}
					} else {
						convertedNode[d] = "";
					}
				}
			}

			nns.push(convertedNode);
		}
		return nns;
	}
	function getWorkspacesAsNodeSet() {
		var nns = [];
		var i;
		for (i in workspaces) {
			if (workspaces.hasOwnProperty(i)) {
				nns.push(workspaces[i]);
			}
		}
		return nns;
	}

	//TODO: rename this (createCompleteNodeSet)
	function createCompleteNodeSet() {
		var nns = [];
		var i;
		for (i in workspaces) {
			if (workspaces.hasOwnProperty(i)) {
				nns.push(workspaces[i]);
			}
		}
		for (i in configNodes) {
			if (configNodes.hasOwnProperty(i)) {
				nns.push(convertNode(configNodes[i], true));
			}
		}
		for (i=0;i<nodes.length;i++) {
			var node = nodes[i];
			nns.push(convertNode(node, true));
		}
		return nns;
	}

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
					var def = node_defs[type];
					var dW = Math.max(RED.view.defaults.width, RED.view.calculateTextWidth(name) + (def.inputs > 0 ? 7 : 0));
					var dH = Math.max(RED.view.defaults.height,(Math.max(def.outputs, def.inputs)||0) * 15);
					var newX = parseInt(coords ? coords[0] : 0);
					var newY = parseInt(coords ? coords[1] : 0);
					//newY = newY == 0 ? lastY + (dH * n) + gap : newY;
					//lastY = Math.max(lastY, newY);
					var node = new Object({"order": n, "id": name, "name": name, "type": type, "x": newX, "y": newY, "z": 0, "wires": []});
					// netter solution: create new id
					if (RED.nodes.node(node.id) !== null) {
						node.z = RED.view.getWorkspace();
						node.id = getID();
						node.name = getUniqueName(node);
					}
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
			$.each(node_defs, function (key, obj) {
				words.push(key);
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
	
	function createNewDefaultWorkspace() // Jannik Add function
	{
		defaultWorkspace = { type:"tab", id:"Main", label:"Main"};//, inputCount:"0", outputCount:"0" };
		addWorkspace(defaultWorkspace);
		RED.view.addWorkspace(defaultWorkspace);
	}
	function convertWorkspaceToNewVersion(nns, ws)
	{
		// TODO: fix so that getClassIOportCounts is only used on old versions, so that load time is increased
		// but in order to do that wee need the workspace inputs and outputs to update when adding/removing workspace IO:s
		// we could insert references to TabInputs/TabOutputs in workspace
		// that would make some things go faster as well

		if (ws.inputs != undefined && ws.outputs != undefined) // (no update from GUI yet) see above
			console.warn("inputs && outputs is defined @ workspace load");
		
			var ws_export; //(cannot use var name export)
		if (ws.export != undefined) {
			//console.warn("export is defined"); // warn is more visible
			ws_export = ws.export;
		} else {
			//console.error("export is undefined");// error is more visible
			ws_export = true; // default value 
		}

		var cIOs = getClassIOportCounts(ws.id, nns);

		return createWorkspaceObject(ws.id, ws.label, cIOs.inCount, cIOs.outCount, ws_export);
	}

	function importNodes(newNodesObj,createNewIds) {
		try {
			var i;
			var n;
			var newNodes;
			if (typeof newNodesObj === "string") {
				if (newNodesObj === "") {
					createNewDefaultWorkspace();
					return;
				}
				newNodes = JSON.parse(newNodesObj);
			} else {
				newNodes = newNodesObj;
			}

			if (!$.isArray(newNodes)) {
				newNodes = [newNodes];
			}
/*			var unknownTypes = [];
			for (i=0;i<newNodes.length;i++) {
				n = newNodes[i];
				// TODO: remove workspace in next release+1
				if (n.type != "workspace" && n.type != "tab" && !getType(n.type)) {
					// TODO: get this UI thing out of here! (see below as well)
					n.name = n.type;
					n.type = "unknown";
					if (unknownTypes.indexOf(n.name)==-1) {
						unknownTypes.push(n.name);
					}
					if (n.x == null && n.y == null) {
						// config node - remove it
						newNodes.splice(i,1);
						i--;
					}
				}
			}

			if (unknownTypes.length > 0) {
				var typeList = "<ul><li>"+unknownTypes.join("</li><li>")+"</li></ul>";
				var type = "type"+(unknownTypes.length > 1?"s":"");
				RED.notify("<strong>Imported unrecognised "+type+":</strong>"+typeList,"error",false,10000);
				//"DO NOT DEPLOY while in this state.<br/>Either, add missing types to Node-RED, restart and then reload page,<br/>or delete unknown "+n.name+", rewire as required, and then deploy.","error");
			}
*/
			for (i=0;i<newNodes.length;i++) { // load workspaces first
				n = newNodes[i];
				// TODO: not remove workspace because it's now used
				if (n.type === "workspace" || n.type === "tab") {
					if (n.type === "workspace") {
						n.type = "tab";
					}
					if (defaultWorkspace == null) {
						defaultWorkspace = n;
					}
					var ws = convertWorkspaceToNewVersion(newNodes, n);
					addWorkspace(ws);
					RED.view.addWorkspace(ws); // final function is in tabs.js
					console.warn("added new workspace lbl:" + n.label + ",inputs:" + n.inputs + ",outputs:" + n.outputs);

					if (ws.inputs != 0 || ws.outputs != 0) // this adds workspaces that have inputs and/or outputs to the palette
					{
						var color = RED.classColor;
						var data = $.parseJSON("{\"defaults\":{\"name\":{\"value\":\"new\"},\"id\":{\"value\":\"new\"}},\"shortName\":\"" + ws.label + "\",\"inputs\":" + ws.inputs + ",\"outputs\":" + ws.outputs + ",\"category\":\"tabs-function\",\"color\":\"" + color + "\",\"icon\":\"arrow-in.png\"}");
						RED.nodes.registerType(ws.label, data);
					}
				}
			}
			if (defaultWorkspace == null) {
				console.log("no default workspace");
				createNewDefaultWorkspace(); // jannik changed to function
			}

			var node_map = {};
			var new_nodes = [];
			var new_links = [];

			for (i=0;i<newNodes.length;i++) {
				n = newNodes[i];
				// TODO: remove workspace in next release+1
				if (n.type !== "workspace" && n.type !== "tab") {
					var def = getType(n.type);
					if (def && def.category == "config") {
						if (!RED.nodes.node(n.id)) {
							var configNode = {id:n.id,type:n.type,users:[]};
							for (var d in def.defaults) {
								if (def.defaults.hasOwnProperty(d)) {
									configNode[d] = n[d];
								}
							}
							configNode.label = def.label;
							configNode._def = def;
							RED.nodes.add(configNode);
						}
					} else {
						var node = {x:n.x,y:n.y,z:n.z,type:0,wires:n.wires,changed:false};
						//console.log("new node:" + n.name + ":" + n.id);
						node.type = n.type;
						node._def = def;
						if (!node._def) {
							node._def = {
								color:"#fee",
								defaults: {},
								label: "unknown: "+n.type,
								labelStyle: "node_label_italic",
								outputs: n.outputs||n.wires.length
							}
						}

						if (createNewIds) {

							node.name = n.name; // we set temporary
							//console.log("@createNewIds srcnode: " + n.id + ":" + n.name);
							if (n.z == RED.view.getWorkspace())
							{
								node.name = createUniqueCppName(node, n.z); // jannik add
								//console.warn("make new name: n.name=" + node.name);
							}
							else
							{
								node.name = n.name;
							
								//console.log("keep name:" + n.name);
							}
							node.id = RED.nodes.cppId(node, RED.nodes.workspaces[RED.view.getWorkspace()].label); // jannik add
							for (var d2 in node._def.defaults) {
								if (node._def.defaults.hasOwnProperty(d2)) {
									if (d2 == "name" || d2 == "id") continue;
									node[d2] = n[d2];
									//console.log("d2: " + d2);
								}
							}

						} else {
							node.name = n.name;
							node.id = n.id;
							if (node.z == null || !workspaces[node.z]) {
								node.z = RED.view.getWorkspace(); // failsafe
							}
							

							for (var d2 in node._def.defaults) {
								if (node._def.defaults.hasOwnProperty(d2)) {
									node[d2] = n[d2];
									//console.log("d2: " + d2);
								}
							}	
						}
						
						node.outputs = n.outputs||node._def.outputs;

						addNode(node);
						RED.editor.validateNode(node);
						node_map[n.id] = node;
						new_nodes.push(node);
					}
				}
			}
			// adding the links (wires)
			for (i=0;i<new_nodes.length;i++) {
				n = new_nodes[i];
				for (var w1=0;w1<n.wires.length;w1++) {
					var wires = (n.wires[w1] instanceof Array)?n.wires[w1]:[n.wires[w1]];
					for (var w2=0;w2<wires.length;w2++) {
						if (wires[w2] != null) {
							var parts = wires[w2].split(":");
							if (parts.length == 2 && parts[0] in node_map) {
								var dst = node_map[parts[0]];
								var link = {source:n,sourcePort:w1,target:dst,targetPort:parts[1]};
								addLink(link);
								new_links.push(link);
							}
						}
					}
				}
				delete n.wires;
			}
			return [new_nodes,new_links];
		} catch(error) { // this try catch is very bad when debugging because you have errors inside somewhere they are hard to find! 
			createNewDefaultWorkspace();
			//TODO: get this UI thing out of here! (see above as well)
			var newException = "<strong>import nodes Error</strong>: "+error.message + " " +  error.stack;
			RED.notify(newException, "error");
			throw newException;
			return null;
		}

	}
	/**
		 * this function checks for !node.wires at beginning and returns if so.
		 * @param {*} srcNode 
		 * @param {*} cb is function pointer with following format cb(srcPortIndex,dstId,dstPortIndex);
		 */
	function eachwire(srcNode, cb) {
		if (!srcNode.wires){ console.log("!node.wires: " + srcNode.type + ":" + srcNode.name); return;}

		//if (srcNode.wires.length == 0) console.log("port.length == 0:" + srcNode.type + ":" + srcNode.name)

		for (var pi=0; pi<srcNode.wires.length; pi++) // pi = port index
		{
			var port = srcNode.wires[pi];
			if (!port){ /*console.log("!port(" + pi + "):" + n.type + ":" + n.name);*/ continue;} // failsafe
			//if (port.length == 0) console.log("portWires.length == 0:"+n.type + ":" + n.name) // debug check
			for (var pwi=0; pwi<port.length; pwi++) // pwi = port wire index
			{
				var wire = port[pwi];
				if (!wire){ /*console.log("!wire(" + pwi + "):" + n.type + ":" + n.name);*/ continue;} // failsafe

				var parts = wire.split(":");
				if (parts.length != 2){ /*console.log("parts.length != 2 (" + pwi + "):" + n.type + ":" + n.name);*/ continue;} // failsafe
				
				var retVal = cb(pi,parts[0],parts[1]);

				if (retVal) return retVal; // only abort/return if cb returns something, and return the value
			}
		}
	}

	function workspaceNameChanged(oldName, newName)
	{
		var changedCount = 0;
		for (var ni=0;ni<nodes.length;ni++) {
			if (nodes[ni].type == oldName)
			{
				nodes[ni].type = newName;
				changedCount++;
			}
		}
		RED.palette.remove(oldName);
		node_defs[oldName] = undefined;

		console.log("workspaceNameChanged:" + oldName + " to " + newName + " with " + changedCount + " objects changed");
	}
	function workspaceNameCheck(newName)
	{
		var i;
		for (i in workspaces) {
			if (workspaces.hasOwnProperty(i)) {
				if (workspaces[i].label == newName)
					return true;
			}
		}
		/*for (var wsi=0; wsi < workspaces.length; wsi++)
		{
			if (workspaces[wsi].label == newName)
				return true;
		}*/
		return false;
	}

	/**
	 * this autogenerate a array-node from same-type selection
	 * caller class is tab-info
	 * @param {*} items is of type moving_set from view
	 */
	function generateArrayNode(items)
	{
		var cppString = "{";
		var type = items[0].n.type;
		var name = type.toLowerCase();
		for (var i = 0; i < items.length ; i++)
		{
			cppString += items[i].n.name;
			if (i < (items.length-1)) cppString += ",";
		}
		cppString += "}";
		addNode({"id":"Main_Array_"+type+"_"+name ,
				 "type":"Array",
				 "name":type + " " + name + " " + cppString,
				 "x":500,"y":55,"z":items[0].n.z,
				 "wires":[],
				 "_def":node_defs["Array"]});
				 RED.view.redraw();
	}
	/**
	 * Gets all TabInput and TabOutputs, and then sorting them vertically top->bottom (normal view)
	 * @param {String} wsId workspace id, if this is not passed then all nodes is returned
	 * @returns {tabOutNodes:outNodes, tabInNodes:inNodes}
	 */
	function getClassIOportsSorted(wsId, nns)
	{
		var inNodes = [];
		var outNodes = [];
		if (!nns)
			nns = nodes;
		for (var i = 0; i < nns.length; i++)
		{
			var node = nns[i];
			if (wsId && (node.z != wsId)) continue;

			if (node.type == "TabInput") inNodes.push(convertNode(node, true));
			else if (node.type == "TabOutput") outNodes.push(convertNode(node, true));
		}
		inNodes.sort(function(a,b){ return (a.y - b.y); });
		outNodes.sort(function(a,b){ return (a.y - b.y); });

		return {outputs:outNodes, inputs:inNodes};
	}
	/**
	 * Gets all TabInput and TabOutputs, and then sorting them vertically top->bottom (normal view)
	 * @param {String} wsId workspace id, if this is not passed then all nodes is returned
	 * @returns {outCount:outNodesCount, inCount:inNodesCount}
	 */
	function getClassIOportCounts(wsId, nns)
	{
		var inNodesCount = 0;
		var outNodesCount = 0;
		if (!nns)
			nns = nodes;
		for (var i = 0; i < nns.length; i++)
		{
			var node = nns[i];
			if (wsId && (node.z != wsId)) continue;

			if (node.type == "TabInput") inNodesCount++;
			else if (node.type == "TabOutput") outNodesCount++;
		}
		return {outCount:outNodesCount, inCount:inNodesCount};
	}
	/**
	 * Gets all TabInput or TabOutput belonging to a class, and then sorting them vertically top->bottom (normal view)
	 * then the correct port-node based by index is returned
	 * @param {String} wsId workspace id
	 * @param {String} type "TabInput" or "TabOutput"
	 * @returns {tabOutNodes:outNodes, tabInNodes:inNodes}
	 */
	function getClassIOportName(wsId, type, index) // this 
	{
		var retNodes = [];
		if (!wsId) return
		for (var i = 0; i < nodes.length; i++)
		{
			var node = nodes[i];
			if (node.z != wsId) continue;
			if (node.type == type) retNodes.push(node);
		}
		retNodes.sort(function(a,b){ return (a.y - b.y); }); // this could be avoided if the io nodes where automatically sorted by default
		return retNodes[index].name;
	}
	function getClassComments(wsId)
	{
		var comment = "";
		for (var i = 0; i < nodes.length; i++)
		{
			var node = nodes[i];
			if (wsId && (node.z != wsId)) continue;

			if (node.type != "ClassComment") continue;

			comment += node.name;
		}
		return comment;
	}

	function printLinks()
	{
		// link structure {source:n,sourcePort:w1,target:dst,targetPort:parts[1]};
		for (var i = 0; i < links.length; i++)
		{
			var link = links[i];
			console.log("createCompleteNodeSet links["+i+"]: " + 
				link.source.name + ":" + link.sourcePort + ", " + link.target.name + ":" + link.targetPort); // debug to see links contents
		}
	}
	function isClass(type)
	{
		var wns = RED.nodes.getWorkspacesAsNodeSet();

		for (var wsi = 0; wsi < wns.length; wsi++)
		{
			var ws = wns[wsi];
			if (type == ws.label) return true;
			//console.log(node.type  + "!="+ ws.label);
		}
		return false;
	}
	
	function getWorkspaceIdFromClassName(type)
	{
		var wns = RED.nodes.getWorkspacesAsNodeSet();

		for (var wsi = 0; wsi < wns.length; wsi++)
		{
			var ws = wns[wsi];
			if (type == ws.label)  return ws.id;
		}
		return "";
	}
	
	/**
	 * This is used to find what is connected to a input-pin
	 * @param {Array} nns array of all nodes
	 * @param {String} wsId workspace id
	 * @param {String} nId node id
	 * @returns {*} as {node:n, srcPortIndex: srcPortIndex}
	 */
	function getWireInputSourceNode(nns, wsId, nId)
	{
		//console.log("try get WireInputSourceNode:" + wsId + ":" + nId);
		for (var ni = 0; ni < nns.length; ni++)
		{
			var n = nns[ni];
			if (n.z != wsId) continue; // workspace check

			var retVal = RED.nodes.eachWire(n, function(srcPortIndex,dstId,dstPortIndex)
			{
				if (dstId == nId)
				{
					//console.log("we found the WireInputSourceNode! name:" + n.name + " ,id:"+ n.id + " ,portIndex:" + srcPortIndex);
					//console.log("");
					return {node:n, srcPortIndex: srcPortIndex};
				}
			});
			if (retVal) return retVal;
		}
	}
	
	/**
	 * the name say it all
	 * @param {Array} tabIOnodes array of specific ClassPort nodes
	 * @param {node} classNode as nodeType
	 * @param {Number} portIndex
	 * @returns {node} the TabInput or TabOutput node
	 */
	function getClassPortNode(tabIOnodes, classNode, portIndex)
	{
		var wsId = getWorkspaceIdFromClassName(classNode.type);
		var currIndex = 0;
		//console.log("getClassPortNode classNode:" + classNode.name + ", portType: " + portType + ", portIndex:" + portIndex);
		for (var i = 0; i < tabIOnodes.length; i++)
		{
			var n = tabIOnodes[i];
			if (n.z != wsId) continue;
			if (currIndex == portIndex) // we found the port
			{
				//console.log("getClassPortNode found port:" + n.name);
				return n;
			}
			currIndex++;
		}
		console.log("ERROR! could not find the class, portType:" + portType + " with portIndex:" + portIndex);
	}
	
	function classOutputPortToCpp(nns, tabOutNodes, ac, classNode)
	{
		var outputNode = getClassPortNode(tabOutNodes, classNode, ac.srcPort);
		if (!outputNode)
		{
			 console.log("could not getClassPortNode:" + classNode.name + ", ac.srcPort:" + ac.srcPort);
			 return false;
		} // abort

		// if the portNode is found, next we get what is connected to that port inside the class
		var newSrc = getWireInputSourceNode(nns, getWorkspaceIdFromClassName(classNode.type), outputNode.id); // this return type {node:n, srcPortIndex: srcPortIndex};

		ac.srcName += "." + make_name(newSrc.node);
		ac.srcPort = newSrc.srcPortIndex;

		if (isClass(newSrc.node.type))
		{
			//console.log("isClass(" + newSrc.node.name + ")");

			// call this function recursive until non class is found
			if (!classOutputPortToCpp(nns, tabOutNodes, ac, newSrc.node))
				return false; // failsafe
		}
		return true;
	}

	function classInputPortToCpp(tabInNodes, currRootName, ac, classNode)
	{
		var inputNode = getClassPortNode(tabInNodes, classNode, ac.dstPort);
		if (!inputNode) return false; // abort

		// here we need to go througt all wires of that virtual port
		var retVal = RED.nodes.eachWire(inputNode, function(srcPortIndex,dstId,dstPortIndex)
		{
			var dst = RED.nodes.node(dstId);
			//console.log("found dest:" + dst.name);
			ac.dstPort = dstPortIndex;
			ac.dstName = currRootName + "." + make_name(dst);

			if (isClass(dst.type))
			{
				// call this function recursive until non class is found
				classInputPortToCpp(tabInNodes, ac.dstName, ac, dst);
			}
			else
			{
				ac.appendToCppCode(); // this don't return anything, the result is in ac.cppCode
			}					
		});
		return true;
	}
	function isNameDeclarationArray(name)
	{
		//console.warn("isNameDeclarationArray: " + name);
		var startIndex = name.indexOf("[");
		if (startIndex == -1) return undefined;
		var endIndex = name.indexOf("]");
		if (endIndex == -1){ console.log("isNameDeclarationArray: missing end ] in " + name); return undefined;}
		var arrayDef = name.substring(startIndex,endIndex+1);
		lenght = Number(name.substring(startIndex+1,endIndex));
		name = name.replace(arrayDef, "[i]");
		
		console.log("NameDeclaration is Array:" + name);
		
		return {newName:name, arrayLenght:lenght};
	}
	function make_name(n) {
		var name = (n.name ? n.name : n.id);
		name = name.replace(" ", "_").replace("+", "_").replace("-", "_");
		return name
	}
	return {
		createWorkspaceObject:createWorkspaceObject,
		createNewDefaultWorkspace: createNewDefaultWorkspace,
		registerType: registerType,
		getType: getType,
		convertNode: convertNode,
		selectNode: selectNode,
		add: addNode,
		addLink: addLink,
		remove: removeNode,
		removeLink: removeLink,
		addWorkspace: addWorkspace,
		removeWorkspace: removeWorkspace,
		workspace: getWorkspace,
		eachNode: function(cb) {
			for (var n=0;n<nodes.length;n++) {
				cb(nodes[n]);
			}
		},
		eachLink: function(cb) {
			for (var l=0;l<links.length;l++) {
				cb(links[l]);
			}
		},
		eachConfig: function(cb) {
			for (var id in configNodes) {
				if (configNodes.hasOwnProperty(id)) {
					cb(configNodes[id]);
				}
			}
		},
		eachWire: eachwire,
		workspaceNameChanged:workspaceNameChanged,
		workspaceNameCheck:workspaceNameCheck,
		node: getNode,
		namedNode: getNodeByName,
		cppToJSON: cppToJSON,
		import: importNodes,
		refreshValidation: refreshValidation,
		getAllFlowNodes: getAllFlowNodes,
		createExportableNodeSet: createExportableNodeSet,
		getWorkspacesAsNodeSet: getWorkspacesAsNodeSet,
		createCompleteNodeSet: createCompleteNodeSet,
		id: getID,
		cppName: createUniqueCppName,
		cppId: createUniqueCppId,
		hasIO: checkForIO,
		generateArrayNode:generateArrayNode,
		isClass:isClass,
		getClassComments:getClassComments,
		getWorkspaceIdFromClassName:getWorkspaceIdFromClassName,

		getClassPortNode:getClassPortNode,
		getWireInputSourceNode:getWireInputSourceNode,
		getClassIOportsSorted:getClassIOportsSorted,
		getClassIOportName:getClassIOportName, // used by node port tooltip popup
		classOutputPortToCpp:classOutputPortToCpp,
		classInputPortToCpp:classInputPortToCpp,
		isNameDeclarationArray:isNameDeclarationArray,
		make_name:make_name,
		showWorkspaceToolbar:showWorkspaceToolbar,
		nodes: nodes, // TODO: exposed for d3 vis
		workspaces:workspaces,
		links: links  // TODO: exposed for d3 vis
	};
})();
