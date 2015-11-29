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
	var links = [];
	var defaultWorkspace;
	var workspaces = {};

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
			console.log("checkID, nodes[i].id = " + nodes[i].id);
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

	function createUniqueCppName(n) {
		console.log("getUniqueCppName, n.type=" + n.type + ", n._def.shortName=" + n._def.shortName);
		var basename = (n._def.shortName) ? n._def.shortName : n.type.replace(/^Analog/, "");
		console.log("getUniqueCppName, using basename=" + basename);
		var count = 1;
		var sep = /[0-9]$/.test(basename) ? "_" : "";
		var name;
		while (1) {
			name = basename + sep + count;
			if (!checkID(name)) break;
			count++;
		}
		console.log("getUniqueCppName, unique name=" + name);
		return name;
	}

	function getType(type) {
		return node_defs[type];
	}
	function selectNode(name) {
	//	window.history.pushState(null, null, window.location.protocol + "//"
	//		+ window.location.host + window.location.pathname + '?info=' + name);
	}
	function addNode(n) {
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
						var type = getType(property.type)
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
	function addConfig(c) {
		configNodes[c.id] = c;
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
						var type = getType(property.type)
						if (type && type.category == "config") {
							var configNode = configNodes[node[d]];
							if (configNode) {
								updatedConfigNode = true;
								var users = configNode.users;
								users.splice(users.indexOf(node), 1);
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

		var data = "";
		var nodes = [];
		var cables = [];

		const CODE_START 	= "// GUItool: begin automatically generated code";
		const CODE_END		= "// GUItool: end automatically generated code";
		const NODE_COMMENT	= "//";
		const NODE_AC		= "AudioConnection";
		const NODE_AI_I2S	= "AudioInputI2S";
		const NODE_AM_4     = "AudioMixer4";
		const NODE_AC_SGTL  = "AudioControlSGTL5000";

		var parseLine = function(line) {
			var parts = line.match(/^(\S+)\s(.*)/).slice(1);
			var type = $.trim(parts[0]);
			line = $.trim(parts[1]) + " ";

			var description = "";
			var coords = [0, 0];
			var conn = [];

			parts = line.match(/^([^;]{0,});(.*)/);
			if (parts) {
				parts = parts.slice(1);
				description = $.trim(parts[0]);
				coords = $.trim(parts[1]);
				parts = coords.match(/^([^\/]{0,})\/\/xy=(.*)/);
				if (parts) {
					parts = parts.slice(1);
					coords = $.trim(parts[1]).split(",");
				}
			}

			switch (type) {
				case NODE_AC:
					parts = description.match(/^([^\(]*\()([^\)]*)(.*)/);
					if (parts) {
						conn = $.trim(parts[2]).split(",");
						cables.push(conn);
					}
					break;
				case NODE_COMMENT:
					// do nothing ...
					break;
				default:
					var node = new Object({
						"id": description,
						"type": type,
						"x": parseInt(coords ? coords[0] : 0),
						"y": parseInt(coords ? coords[1] : 0),
						"z": 0,
						"wires": []
					});
					nodes.push(node);
					break;
			}
		};

		var findNode = function(name) {
			var len = nodes.length;
			for (var key = 0; key < len; key++) {
				if (nodes[key].id == name) {
					return nodes[key];
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
			var useLine = 0;

			for (var i = 0; i < lines.length; i++) {
				var line = lines[i];
				useLine += (line.indexOf(CODE_START) >= 0) ? (useLine ? 0 : 1) : 0;
				if (useLine > 0) {
					parseLine(line);
				}
				useLine -= (line.indexOf(CODE_END) >= 0) ? (useLine ? 1 : 0) : 0;
			}
		};

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

		traverseLines(newNodesStr);
		linkCables(cables);

		return JSON.stringify(nodes);
	}

	function importNodes(newNodesObj,createNewIds) {
		try {
			var i;
			var n;
			var newNodes;
			if (typeof newNodesObj === "string") {
				if (newNodesObj === "") {
					return;
				}
				newNodes = JSON.parse(newNodesObj);
			} else {
				newNodes = newNodesObj;
			}

			if (!$.isArray(newNodes)) {
				newNodes = [newNodes];
			}
			var unknownTypes = [];
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
/*
			if (unknownTypes.length > 0) {
				var typeList = "<ul><li>"+unknownTypes.join("</li><li>")+"</li></ul>";
				var type = "type"+(unknownTypes.length > 1?"s":"");
				RED.notify("<strong>Imported unrecognised "+type+":</strong>"+typeList,"error",false,10000);
				//"DO NOT DEPLOY while in this state.<br/>Either, add missing types to Node-RED, restart and then reload page,<br/>or delete unknown "+n.name+", rewire as required, and then deploy.","error");
			}

			for (i=0;i<newNodes.length;i++) {
				n = newNodes[i];
				// TODO: remove workspace in next release+1
				if (n.type === "workspace" || n.type === "tab") {
					if (n.type === "workspace") {
						n.type = "tab";
					}
					if (defaultWorkspace == null) {
						defaultWorkspace = n;
					}
					addWorkspace(n);
					RED.view.addWorkspace(n);
				}
			}
			if (defaultWorkspace == null) {
				defaultWorkspace = { type:"tab", id:getID(), label:"Sheet 1" };
				addWorkspace(defaultWorkspace);
				RED.view.addWorkspace(defaultWorkspace);
			}
*/
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
						if (createNewIds) {
							node.z = RED.view.getWorkspace();
							node.id = getID();
						} else {
							node.id = n.id;
							if (node.z == null || !workspaces[node.z]) {
								node.z = RED.view.getWorkspace();
							}
						}
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
						node.outputs = n.outputs||node._def.outputs;

						for (var d2 in node._def.defaults) {
							if (node._def.defaults.hasOwnProperty(d2)) {
								node[d2] = n[d2];
							}
						}

						addNode(node);
						RED.editor.validateNode(node);
						node_map[n.id] = node;
						new_nodes.push(node);
					}
				}
			}
			for (i=0;i<new_nodes.length;i++) {
				n = new_nodes[i];
				for (var w1=0;w1<n.wires.length;w1++) {
					var wires = (n.wires[w1] instanceof Array)?n.wires[w1]:[n.wires[w1]];
					for (var w2=0;w2<wires.length;w2++) {
						var parts = wires[w2].split(":");
						if (parts.length == 2 && parts[0] in node_map) {
							var dst = node_map[parts[0]];
							var link = {source:n,sourcePort:w1,target:dst,targetPort:parts[1]};
							addLink(link);
							new_links.push(link);
						}
					}
				}
				delete n.wires;
			}
			return [new_nodes,new_links];
		} catch(error) {
			//TODO: get this UI thing out of here! (see above as well)
			RED.notify("<strong>Error</strong>: "+error,"error");
			return null;
		}

	}

	return {
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
		node: getNode,
		import: importNodes,
		cppToJSON: cppToJSON,
		refreshValidation: refreshValidation,
		getAllFlowNodes: getAllFlowNodes,
		createExportableNodeSet: createExportableNodeSet,
		createCompleteNodeSet: createCompleteNodeSet,
		id: getID,
		cppName: createUniqueCppName,
		nodes: nodes, // TODO: exposed for d3 vis
		links: links  // TODO: exposed for d3 vis
	};
})();
