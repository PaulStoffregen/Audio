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
RED.arduino.export = (function() {
    /**
	 * this take a multiline text, 
	 * break it up into linearray, 
	 * then each line is added to a new text + the incrementText added in front of every line
	 * @param {*} text 
	 * @param {*} incrementText 
	 */
	function incrementTextLines(text, incrementText)
	{
		var lines = text.split("\n");
		var newText = "";
		for (var i = 0; i < lines.length; i++)
		{
			newText += incrementText + lines[i] + "\n";
		}
		return newText;
	}
    
    $('#btn-deploy').click(function() { save(); });
	function save(force) {
		RED.storage.update();

		if (RED.nodes.hasIO()) {
			var nns = RED.nodes.createCompleteNodeSet();
			// sort by horizontal position, plus slight vertical position,
			// for well defined update order that follows signal flow
			nns.sort(function(a,b){ return (a.x + a.y/250) - (b.x + b.y/250); });
			//console.log(JSON.stringify(nns));

			var cpp = "#include <Audio.h>\n#include <Wire.h>\n"
				+ "#include <SPI.h>\n#include <SD.h>\n#include <SerialFlash.h>\n\n"
				+ "// GUItool: begin automatically generated code\n";
				+ "// JSON string:\n"
				+ "//" + JSON.stringify(nns) + "\n";
				
			// generate code for all audio processing nodes
			for (var i=0; i<nns.length; i++) {
				var n = nns[i];
				var node = RED.nodes.node(n.id);
				if (node && (node.outputs > 0 || node._def.inputs > 0)) {
					cpp += n.type + " ";
					for (var j=n.type.length; j<24; j++) cpp += " ";
					var name = make_name(n)
					cpp += name + "; ";
					for (var j=n.id.length; j<14; j++) cpp += " ";
					cpp += "//xy=" + n.x + "," + n.y + "\n";
				}
			}
			// generate code for all connections (aka wires or links)
			var cordcount = 1;
			for (var i=0; i<nns.length; i++) {
				var n = nns[i];
				if (n.wires) {
					for (var j=0; j<n.wires.length; j++) {
						var wires = n.wires[j];
						if (!wires) continue;
						for (var k=0; k<wires.length; k++) {
							var wire = n.wires[j][k];
							if (wire) {
								var parts = wire.split(":");
								if (parts.length == 2) {
									cpp += "AudioConnection          patchCord" + cordcount + "(";
									var src = RED.nodes.node(n.id);
									var dst = RED.nodes.node(parts[0]);
									var src_name = RED.nodes.make_name(src);
									var dst_name = RED.nodes.make_name(dst);
									if (j == 0 && parts[1] == 0 && src && src.outputs == 1 && dst && dst._def.inputs == 1) {
										cpp += src_name + ", " + dst_name;
									} else {
										cpp += src_name + ", " + j + ", " + dst_name + ", " + parts[1];
									}
									cpp += ");\n";
									cordcount++;
								}
							}
						}
					}
				}
			}
			// generate code for all control nodes (no inputs or outputs)
			for (var i=0; i<nns.length; i++) {
				var n = nns[i];
				var node = RED.nodes.node(n.id);
				if (node && node.outputs == 0 && node._def.inputs == 0) {
					cpp += n.type + " ";
					for (var j=n.type.length; j<24; j++) cpp += " ";
					cpp += n.id + "; ";
					for (var j=n.id.length; j<14; j++) cpp += " ";
					cpp += "//xy=" + n.x + "," + n.y + "\n";
				}
			}
			cpp += "// GUItool: end automatically generated code\n";
			//console.log(cpp);
			httpPostAsync(cpp);
			RED.view.state(RED.state.EXPORT);
			RED.view.getForm('dialog-form', 'export-clipboard-dialog', function (d, f) {
				$("#node-input-export").val(cpp).focus(function() {
				var textarea = $(this);
				textarea.select();
				textarea.mouseup(function() {
					textarea.unbind("mouseup");
					return false;
				});
				}).focus();
			$( "#dialog" ).dialog("option","title","Export to Arduino").dialog( "open" );
			});
			//RED.view.dirty(false);
		} else {
			$( "#node-dialog-error-deploy" ).dialog({
				title: "Error exporting data to Arduino IDE",
				modal: true,
				autoOpen: false,
				width: 410,
				height: 245,
				buttons: [{
					text: "Ok",
					click: function() {
						$( this ).dialog( "close" );
					}
				}]
			}).dialog("open");
		}
	}
    function isSpecialNode(type)
	{
		if (type == "ClassComment") return true;
		else if (type == "Comment") return true;
		else if (type == "TabInput") return true;
		else if (type == "TabOutput") return true;
		else if (type == "Function") return true;
		else if (type == "Variables") return true;
		else return false;
	}
	$('#btn-deploy2').click(function() { save2(); });
	function save2(force)
	{
		RED.storage.update();

		if (RED.nodes.hasIO())
		{
			var nns = RED.nodes.createCompleteNodeSet();

			var tabNodes = RED.nodes.getClassIOportsSorted();

			// sort by horizontal position, plus slight vertical position,
			// for well defined update order that follows signal flow
			nns.sort(function(a,b){ return (a.x/50 + a.y/250) - (b.x/50 + b.y/250); }); // 50 is the visual major-vertical-gridsize
			//console.log(JSON.stringify(nns)); // debug test

			var cpp = "#include <Audio.h>\n#include <Wire.h>\n"
				+ "#include <SPI.h>\n#include <SD.h>\n#include <SerialFlash.h>\n\n"
				+ "\n// GUItool: begin automatically generated code\n"
				+ "// JSON string:\n"
				+ "//" + JSON.stringify(nns) + "\n";

			for (var wsi=0; wsi < RED.nodes.workspaces.length; wsi++) // workspaces
			{
				var ws = RED.nodes.workspaces[wsi];
				if (!ws.export) continue; // this skip export
				
				// first go through special types
				var classComment = "";
				var classFunctions = "";
				var classVars = "";
				var arrayNode = undefined;
				var foundArrayNode = false;
				for (var i=0; i<nns.length; i++) { 
					var n = nns[i];

					if (n.z != ws.id) continue; // workspace filter
					if (n.type == "ClassComment")
					{
						//if (n.name == "TestMultiline")
						//	RED.nodes.node(n.id).name = "Test\nMultiline";
						classComment += " * " + n.name + "\n";
					}
					else if (n.type == "Function")
					{
						classFunctions += n.comment + "\n"; // we use comment field for function-data
					}
					else if (n.type == "Variables")
					{
						classVars += n.comment + "\n" // we use comment field for vars-data
					}
					else if (n.type == "Array" && !foundArrayNode) // this is special thingy that was before real-node, now it's obsolete, it only generates more code
					{
						var arrayNode = node.name.split(" ");
						if (!arrayNode) continue;
						if (arrayNode.length < 2 || arrayNode.length > 3) continue;
						// we just save the array def. to later
						if (arrayNode.length == 2)
							arrayNode = {type:arrayNode[0], name:arrayNode[1], cppCode:"", objectCount:0, autoGenerate:true}; 
						else // arrayNode[2] contains predefined array contents
							arrayNode = {type:arrayNode[0], name:arrayNode[1], cppCode:arrayNode[2], objectCount:arrayNode[2].split(",").length, autoGenerate:false};

						cpp += "    " + arrayNode.type + " ";
						for (var j=arrayNode.type.length; j<32; j++) cpp += " ";
						cpp += "*" + arrayNode.name +";\n";
						
						foundArrayNode = true; // only one can be defined at this beta test
					}
				}
				if (classComment.length > 0)
				{
					cpp += "\n/**\n" + classComment + " */"; // newline not needed because it allready in beginning of class definer (check down)
				}
				cpp += "\nclass " + ws.label + "\n{\n public:\n";

				//cpp += "// " + wns[i].id + ":" + wns[i].label + "\n"; // test 
				
				// generate code for all audio processing nodes
				for (var i=0; i<nns.length; i++) {
					var n = nns[i];

					if (n.z != ws.id) continue; // workspace filter

					var node = RED.nodes.node(n.id);
					if (!node) continue;
					
					if(isSpecialNode(n.type)) continue;

					if ((node.outputs <= 0) && (node._def.inputs <= 0)) continue;
					cpp += "    "
					//console.log(">>>" + n.type +"<<<"); // debug test
					var typeLength = n.type.length;
					if (n.type == "AudioMixer")
					{
						var tmplDef = "";
						if (n.inputs == 1) // special case 
						{
							// check if source is a array
							var src = RED.nodes.getWireInputSourceNode(nns, n.z, n.id);
							if (!src.node.name)
							{
								console.error("!src.node:" + n.z + ":" + n.id); // failsafe only happens when no name is used for an node
								continue;
							}
							var isArray = RED.nodes.isNameDeclarationArray(src.node.name);
							if (isArray) tmplDef = "<" + isArray.arrayLenght + ">";
							console.log("special case AudioMixer connected from array " + src.node.name + ", new AudioMixer def:" + tmplDef);
						}
						else
							tmplDef = "<" + n.inputs + ">";

						cpp += n.type + tmplDef + " ";
						typeLength += tmplDef.length;
					}
					else
						cpp += n.type + " ";

					for (var j=typeLength; j<32; j++) cpp += " ";
					var name = RED.nodes.make_name(n)

					if (arrayNode && arrayNode.autoGenerate && (n.type == arrayNode.type))
					{
						arrayNode.cppCode += name + ",";
						arrayNode.objectCount++;
					}
					if (n.comment && (n.comment.trim().length != 0))
						cpp += name + "; /* " + n.comment +"*/\n";
					else
						cpp += name + ";\n";
					
				}
				
				// generate code for all control nodes (no inputs or outputs)
				for (var i=0; i<nns.length; i++) {
					var n = nns[i];

					if (n.z != ws.id) continue;

					var node = RED.nodes.node(n.id);
					if (node && node.outputs == 0 && node._def.inputs == 0) {

						if(isSpecialNode(n.type)) continue;

						cpp += "    " + n.type + " ";
						for (var j=n.type.length; j<32; j++) cpp += " ";
						var name = RED.nodes.make_name(n)
						cpp += name + ";\n";
						//for (var j=n.name.length; j<26; j++) cpp += " ";
						//cpp += "//xy=" + n.x + "," + n.y + "," + n.z + "\n"; // now with JSON string at top xy not needed anymore
						//cpp+= "\n";
					}
				}
				
				// generate code for all connections (aka wires or links)
				var ac = {
					base: "        AudioConnection        patchCord",
					arrayBase: "        patchCord[pci++] = new AudioConnection(",
					dstRootIsArray: false,
					srcRootIsArray: false,
					arrayLenght: 0,
					srcName: "",
					srcPort: 0,
					dstName: "",
					dstPort: 0,
					count: 1,
					totalCount: 0,
					cppCode: "",
					ifAnyIsArray: function() {
						return (this.dstRootIsArray || this.srcRootIsArray);
					},
					appendToCppCode: function() {
						//if ((this.srcPort == 0) && (this.dstPort == 0))
						//	this.cppCode	+= "\n" + this.base + this.count + "(" + this.srcName + ", " + this.dstName + ");"; // this could be used but it's generating code that looks more blurry
						
						if (this.dstRootIsArray)
						{
							this.cppCode	+= "    " + this.arrayBase + this.srcName + ", " + this.srcPort + ", " + this.dstName + ", " + this.dstPort + ");\n";
							this.totalCount+=this.arrayLenght;
						}
						else if (this.srcRootIsArray)
						{
							this.cppCode	+= "    " + this.arrayBase + this.srcName + ", " + this.srcPort + ", " + this.dstName + ", i);\n";
							this.totalCount+=this.arrayLenght;
						}
						else 
						{
							this.cppCode	+= this.arrayBase + this.srcName + ", " + this.srcPort + ", " + this.dstName + ", " + this.dstPort + ");\n";
							this.count++;
							this.totalCount++;
						}
						
					},
					checkIfDstIsArray: function() {
						var isArray = RED.nodes.isNameDeclarationArray(this.dstName);
						if (!isArray)
						{
							this.dstRootIsArray = false;
							return false;
						}
						this.arrayLenght = isArray.arrayLenght;
						this.dstName = isArray.newName;
						this.dstRootIsArray = true;
						return true;
					},
					checkIfSrcIsArray: function() {
						
						var isArray = RED.nodes.isNameDeclarationArray(this.srcName);
						if (!isArray)
						{
							this.srcRootIsArray = false;
							return false;
						}
						this.arrayLenght = isArray.arrayLenght;
						this.srcName = isArray.newName;
						this.srcRootIsArray = true;
						return true;
					}
				};
				ac.count = 1;
				var cppPcs = "";
				var cppArray = "";
				for (var i=0; i<nns.length; i++)
				{
					var n = nns[i];

					if (n.z != ws.id) continue; // workspace check
					
					RED.nodes.eachWire(n, function (pi, dstId, dstPortIndex)
					{
						var src = RED.nodes.node(n.id);
						var dst = RED.nodes.node(dstId);

						if (src.type == "TabInput" || dst.type == "TabOutput") return; // now with JSON string at top, place-holders not needed anymore
							
						ac.cppCode = "";
						ac.srcName = RED.nodes.make_name(src);
						ac.dstName = RED.nodes.make_name(dst);
						ac.srcPort = pi;
						ac.dstPort = dstPortIndex;

						ac.checkIfSrcIsArray(); // we ignore the return value, there is no really use for it
						if (RED.nodes.isClass(n.type)) // if source is class
						{
							//console.log("root src is class:" + ac.srcName);
							//classOutputPortToCpp(nns, outNodes, ac, n); // debug
							RED.nodes.classOutputPortToCpp(nns, tabNodes.outputs, ac, n);
						}
						
						ac.checkIfDstIsArray(); // we ignore the return value, there is no really use for it
						if (RED.nodes.isClass(dst.type))
						{							
							//console.log("dst is class:" + dst.name + " from:" + n.name);
							//classInputPortToCpp(inNodes, ac.dstName , ac, dst); // debug;
							RED.nodes.classInputPortToCpp(tabNodes.inputs, ac.dstName , ac, dst);
						}
						else
						{
							ac.appendToCppCode(); // this don't return anything, the result is in ac.cppCode
						}
						if (ac.ifAnyIsArray())
							cppArray += ac.cppCode;
						else
							cppPcs += ac.cppCode;
					});
				}
				cpp += "    AudioConnection ";
				for (var j="AudioConnection".length; j<32; j++) cpp += " ";
				cpp += "*patchCord[" + ac.totalCount + "]; // total patchCordCount:" + ac.totalCount + " including array typed ones.\n";
				if (classVars.trim().length > 0)
					cpp += "\n" + incrementTextLines(classVars, "    ");
				cpp+= "\n    " + ws.label + "() // constructor (this is called when class-object is created)\n    {\n";
				cpp += "        int pci = 0; // used only for adding new patchcords\n\n"

				if (arrayNode) // if defined and found prev, add it now
				{
					cpp += "        // array workaround until we get real object-array in GUI-tool\n";
					cpp += "        " + arrayNode.name + " = new " + arrayNode.type + "[" + arrayNode.objectCount + "]";
					if (arrayNode.autoGenerate)
						cpp += "{" + arrayNode.cppCode.substring(0, arrayNode.cppCode.length - 1) + "};\n\n"
					else
						cpp += arrayNode.cppCode + ";\n\n"
					
				}
				cpp += cppPcs;
				if (ac.arrayLenght != 0)
				{
					cpp += "        for (int i = 0; i < " + ac.arrayLenght + "; i++)\n        {\n";
					cpp += cppArray;
					cpp += "        }\n";
				}
				cpp += "    }\n";
				if (classFunctions.trim().length > 0)
					cpp += "\n" + incrementTextLines(classFunctions, "    ");
				cpp += "};\n"; // end of class
			}
			cpp += "// GUItool: end automatically generated code\n";
			//console.log(cpp);
			RED.arduino.httpPostAsync(cpp);
			
			var box = document.querySelector('.ui-droppable');
			function float2int (value) {
				return value | 0;
			}
			RED.view.state(RED.state.EXPORT);
			RED.view.getForm('dialog-form', 'export-clipboard-dialog', function (d, f) {
				
				$("#node-input-export").val(cpp).focus(function() {
				var textarea = $(this);
				
				textarea.select();
				//console.error(textarea.height());
				var textareaNewHeight = float2int((box.clientHeight-220)/20)*20;// 20 is the calculated text line height @ 12px textsize, 220 is the offset
				textarea.height(textareaNewHeight);

				textarea.mouseup(function() {
					textarea.unbind("mouseup");
					return false;
				});
				}).focus();

				/*$("#node-input-export2").val("second text").focus(function() { // this can be used for additional setup loop code in future
					// future is now and with direct communication to from arduino ide this is no longer needed.
					var textarea = $(this);
					textarea.select();
					textarea.mouseup(function() {
						textarea.unbind("mouseup");
						return false;
					});
					}).focus();*/ 
					
					console.warn(".ui-droppable.box.clientHeight:"+ box.clientHeight);
			//$( "#dialog" ).dialog("option","title","Export to Arduino").dialog( "open" );
				$( "#dialog" ).dialog({
					title: "Class Export to Arduino",
					modal: true,
					autoOpen: false,
					width: box.clientWidth*0.60 ,
					height: box.clientHeight }).dialog( "open" );
				
			});
			//RED.view.dirty(false);
		} else {
			$( "#node-dialog-error-deploy" ).dialog({
				title: "Error exporting data to Arduino IDE",
				modal: true,
				autoOpen: false,
				width: 410,
				height: 245,
				buttons: [{
					text: "Ok",
					click: function() {
						$( this ).dialog( "close" );
					}
				}]
			}).dialog("open");
		}
    }

    return {
        isSpecialNode:isSpecialNode,
	};
})();