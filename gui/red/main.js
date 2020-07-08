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
var RED = (function() {

	//var classColor = "#E6E0F8"; // standard
	var classColor = "#ccffcc"; // new

	$('#btn-keyboard-shortcuts').click(function(){showHelp();});

	function hideDropTarget() {
		$("#dropTarget").hide();
		RED.keyboard.remove(/* ESCAPE */ 27);
	}

// Jannik comment out, because there is no use of this function
  //  we could maybe use for dropping new nodes
  //  but in that case it's only dropTarget we use
	$('#chart').on("dragenter",function(event) {
		if ($.inArray("text/plain",event.originalEvent.dataTransfer.types) != -1) {
			$("#dropTarget").css({display:'table'});
			RED.keyboard.add( 27,hideDropTarget); // ESCAPE
		}
	});

	$('#dropTarget').on("dragover",function(event) {
		if ($.inArray("text/plain",event.originalEvent.dataTransfer.types) != -1) {
			event.preventDefault();
		}
	})
	.on("dragleave",function(event) {
		hideDropTarget();
	})
	.on("drop",function(event) {
		var data = event.originalEvent.dataTransfer.getData("text/plain");
		
		hideDropTarget();
		if (data.startsWith("file")) return;
		console.log("flow dropped:" + data);
		RED.view.importNodes(data);
		event.preventDefault();
	});


	function make_name(n) {
		var name = (n.name ? n.name : n.id);
		name = name.replace(" ", "_").replace("+", "_").replace("-", "_");
		return name
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
	$('#btn-deploy2').click(function() { save2(); });
	function save2(force)
	{
		//TODO: to use this following sort, for a well defined json result
		//it's more meaningfull if we first sort nodes by workspace
		//RED.nodes.nodes.sort(function(a,b){ return (a.x + a.y/250) - (b.x + b.y/250); }); 
		//maybe it's better if we do the sorting in RED.storage.update();

		RED.storage.update();

		if (RED.nodes.hasIO())
		{
			var nns = RED.nodes.createCompleteNodeSet();

			var tabNodes = RED.nodes.getClassIOportsSorted();

			// sort by horizontal position, plus slight vertical position,
			// for well defined update order that follows signal flow
			nns.sort(function(a,b){ return (a.x + a.y/250) - (b.x + b.y/250); });
			//console.log(JSON.stringify(nns)); // debug test

			var wns = RED.nodes.getWorkspacesAsNodeSet();

			var cpp = "#include <Audio.h>\n#include <Wire.h>\n"
				+ "#include <SPI.h>\n#include <SD.h>\n#include <SerialFlash.h>\n\n"
				+ "\n// GUItool: begin automatically generated code\n"
				+ "// JSON string:\n"
				+ "//" + JSON.stringify(nns) + "\n";
			
				
				
			for (var wsi=0; wsi < wns.length; wsi++)
			{
				if (!wns[wsi].export) continue; // this skip export

				var classComment = "";

				
				for (var i=0; i<nns.length; i++) {
					var n = nns[i];

					if (n.z != wns[wsi].id) continue;
					if (n.type == "ClassComment")
					{
						//if (n.name == "TestMultiline")
						//	RED.nodes.node(n.id).name = "Test\nMultiline";
						classComment += " * " + n.name + "\n";
					}
				}
				if (classComment.length > 0)
				{
					cpp += "\n/**\n" + classComment + " */"; // newline not needed because it allready in beginning of class definer (check down)
				}

				cpp += "\nclass " + wns[wsi].label + "\n{\n"
				+ "  public:\n";

				//console.log("class>>>" + wns[wsi].label + "<<<"); // debug test
			
				//cpp += "// " + wns[i].id + ":" + wns[i].label + "\n"; // test 
				var arrayNode = undefined;
				// generate code for the array def. node, at the moment there can only be one
				for (var i=0; i<nns.length; i++) {
					var n = nns[i];

					if (n.z != wns[wsi].id) continue; // workspace check

					var node = RED.nodes.node(n.id);
					if (!node) continue;
					if (node.type != "Array") continue;
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
					
					break; // only one can be defined at this beta test

					//for (var j=arrayNode[1].length; j<26; j++) cpp += " ";

					//cpp += "//xy=" + n.x + "," + n.y + "," + n.z + "\n"; // now with JSON string at top xy not needed anymore
				}
				// generate code for all audio processing nodes
				for (var i=0; i<nns.length; i++) {
					var n = nns[i];

					if (n.z != wns[wsi].id) continue; // workspace check

					var node = RED.nodes.node(n.id);
					if (!node) continue;
					
					if (n.type == "TabInput" || n.type == "TabOutput") continue; // now with JSON string at top place-holders not needed anymore
					if (n.type == "Array") continue;

					if ((node.outputs <= 0) && (node._def.inputs <= 0)) continue;
						
					

					//	cpp += "//  "; // comment out because this is just a placeholder for import
					//else
						cpp += "    "
						
					//console.log(">>>" + n.type +"<<<"); // debug test
					var typeLength = n.type.length;
					if (n.type == "AudioMixerX")
					{
						var tmplDef = "";
						if (n.inputs == 1) // special case 
						{
							// check if source is a array
							var src = RED.nodes.getWireInputSourceNode(nns, n.z, n.id);
							if (!src.node.name)
							{
								
								console.error("!src.node:" + n.z + ":" + n.id);
								continue;
							}
							var isArray = RED.nodes.isNameDeclarationArray(src.node.name);
							if (isArray) tmplDef = "<" + isArray.arrayLenght + ">";
							console.log("special case AudioMixerX connected from array " + src.node.name + ", new AudioMixerX def:" + tmplDef);
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

					if (n.z != wns[wsi].id) continue;

					var node = RED.nodes.node(n.id);
					if (node && node.outputs == 0 && node._def.inputs == 0) {

						if (n.type == "Array") continue; // skip this, it's allready added above
						if (n.type == "ClassComment" || n.type == "Comment") continue;

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
				//var cordcount = 1;

				
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

					if (n.z != wns[wsi].id) continue; // workspace check
					
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

						if (ac.checkIfSrcIsArray())
						{

						}
						if (RED.nodes.isClass(n.type)) // if source is class
						{
							//console.log("root src is class:" + ac.srcName);
							//classOutputPortToCpp(nns, outNodes, ac, n); // debug
							RED.nodes.classOutputPortToCpp(nns, tabNodes.outputs, ac, n);
						}
						
						if (ac.checkIfDstIsArray())
						{

						}
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
				//cpp += "    // total patchCordCount: " + ac.totalCount + " including array typed ones.\n";
				cpp += "    AudioConnection ";
				for (var j="AudioConnection".length; j<32; j++) cpp += " ";
				cpp += "*patchCord[" + ac.totalCount + "]; // total patchCordCount:" + ac.totalCount + " including array typed ones.\n";
				
				cpp+= "\n    " + wns[wsi].label + "() // constructor (this is called when class-object is created)\n    {\n";
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
				cpp += "};\n"; // end of class
			}
			cpp += "// GUItool: end automatically generated code\n";
			//console.log(cpp);

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
	
	function download(filename, text) {
		var element = document.createElement('a');
		element.setAttribute('href', 'data:text/plain;charset=utf-8,' + encodeURIComponent(text));
		element.setAttribute('download', filename);
	  
		element.style.display = 'none';
		document.body.appendChild(element);
	  
		element.click();
	  
		document.body.removeChild(element);
	}
	function readSingleFile(e) {
		var file = e.target.files[0];
		if (!file) {
		  return;
		}
		var reader = new FileReader();
		reader.onload = function(e) {
		  var contents = e.target.result;
		  // Display file content
		  displayContents(contents);
		};
		reader.readAsText(file);
	  }
	   
	  function displayContents(contents) {
		//var element = document.getElementById('file-content');
		RED.storage.loadFile(contents);
	  }
	   
	  document.getElementById('file-input').addEventListener('change', readSingleFile, false);

	  $('#btn-loadFromFile').click(function() { loadFromFile(); });
	  function loadFromFile()
	  {
		
	  }
	$('#btn-saveTofile').click(function() { saveAsFile(); });
	function saveAsFile()
	{
		var nns = RED.nodes.createCompleteNodeSet();
		var jsonString  = JSON.stringify(nns, null, 4);
		download("TeensyAudioDesign.json", jsonString);
	}
	/**
	 * function used by addClassTabsToPalette()
	 */
	function getClassNrOfInputs(nns, classUid)// Jannik add function
	{
		var count = 0;
		for (var i = 0; i  < nns.length; i++)
		{
			var n = nns[i];
			if (n.z == classUid)
			{
				if (n.type == "TabInput")
				{
					count++;
					//console.log("TabInput:" + n.name);
				}
			}
		}
		return count;
	}
	/**
	 * function used by addClassTabsToPalette()
	 */
	function getClassNrOfOutputs(nns, classUid)// Jannik add function
	{
		var count = 0;
		for (var i = 0; i  < nns.length; i++)
		{
			var n = nns[i];
			if (n.z == classUid)
			{
				if (n.type == "TabOutput")
				{
					count++;
					//console.log("TabOutput:" + n.name);
				}
			}
		}
		return count;
	}
	
	function refreshClassNodes()// Jannik add function
	{
	   var nns = RED.nodes.createCompleteNodeSet();
	   var wns = RED.nodes.getWorkspacesAsNodeSet(); // so that we can get class names
	   nns.sort(function(a,b){ return (a.x/250 + a.y/250) - (b.x/250 + b.y/250); });

	   for ( var wsi = 0; wsi < wns.length; wsi++)
	   {
		   var ws = wns[wsi];
		   //console.log("ws.label:" + ws.label); // debug
		   for ( var ni = 0; ni < nns.length; ni++)
		   {
			   var n = nns[ni];
			   //console.log("n.type:" + n.type); // debug
			   if (n.type == ws.label)
			   {
				   console.log("updating " + n.type);
					var node = RED.nodes.node(n.id); // debug
					node._def = RED.nodes.getType(n.type); // refresh type def
					if (!node._def)
					{
						console.error("@refreshClassNodes: node._def is undefined!!!")
						continue;
					}
					node._def.inputs = getClassNrOfInputs(nns, ws.id);
					
					node._def.outputs = getClassNrOfOutputs(nns, ws.id); // this dont really work directly
					node.outputs = node._def.outputs; // this will update current

					node.resize = true; // trigger redraw of ports
			   }
		   }
	   }
	}
	function addClassTabsToPalette()// Jannik add function
	{
		var wns = RED.nodes.getWorkspacesAsNodeSet();
		var nns = RED.nodes.createCompleteNodeSet();
		// sort by horizontal position, plus slight vertical position,
		// for well defined update order that follows signal flow
		nns.sort(function(a,b){ return (a.x/250 + a.y/250) - (b.x/250 + b.y/250); });

		for (var i=0; i < wns.length; i++)
		{
			var ws = wns[i];
			var inputCount = getClassNrOfInputs(nns, ws.id);
			var outputCount = getClassNrOfOutputs(nns, ws.id);

			if ((inputCount == 0) && (outputCount == 0)) continue; // skip adding class with no IO

			
			//var data = $.parseJSON("{\"defaults\":{\"name\":{\"value\":\"new\"}},\"shortName\":\"" + ws.label + "\",\"inputs\":" + inputCount + ",\"outputs\":" + outputCount + ",\"category\":\"tabs-function\",\"color\":\"" + classColor + "\",\"icon\":\"arrow-in.png\"}");
			var data = $.parseJSON("{\"defaults\":{\"name\":{\"value\":\"new\"},\"id\":{\"value\":\"new\"}},\"shortName\":\"" + ws.label + "\",\"inputs\":" + inputCount + ",\"outputs\":" + outputCount + ",\"category\":\"tabs-function\",\"color\":\"" + classColor + "\",\"icon\":\"arrow-in.png\"}");

			RED.nodes.registerType(ws.label, data);
		}
	}
	function getConfirmLoadDemoText(filename)
	{
		return "<p> You are going to replace your current flow</p>" +
			   "<p> with " + filename + ".</p>" +
			   "<p>Are you sure you want to load?</p>" +
			   "<p>Note. your current design will be downloaded as TeensyAudioDesign.json</p>";
	}
	$('#btn-demoFlowA').click(function() {
		var data = $("script[data-container-name|='DemoFlowA']").html();
		console.error("load demo A");
		//console.log(data);
		verifyDialog("Confirm Load", "!!!WARNING!!!", getConfirmLoadDemoText("DemoFlowA"), function(okPressed) { 
			if (okPressed)
			{
				saveAsFile();
				RED.storage.loadFile(data);
			}
		});
	});

	$('#btn-demoFlowB').click(function() {
		var data = $("script[data-container-name|='DemoFlowB']").html();
		console.error("load demo B");
		//console.log(data);
		verifyDialog("Confirm Load", "!!!WARNING!!!", getConfirmLoadDemoText("DemoFlowB"), function(okPressed) { 
			if (okPressed)
			{
				saveAsFile();
				RED.storage.loadFile(data);
			}
		});
		
	});
	$('#btn-originalFlow').click(function() {
		var data = $("script[data-container-name|='FlowOriginal']").html();
		console.log(data);
		verifyDialog("Confirm Load", "!!!WARNING!!!", getConfirmLoadDemoText("FlowOriginal"), function(okPressed) { 
			if (okPressed)
			{
				saveAsFile();
				RED.storage.loadFile(data);
			}
		});
		
	});

	function verifyDialog(dialogTitle, textTitle, text, cb) {
		$( "#node-dialog-verify" ).dialog({
			modal: true,
			autoOpen: false,
			width: 500,
			title: dialogTitle,
			buttons: [
				{ text: "Ok", click: function() { cb(true); $( this ).dialog( "close" );	} },
				{ text: "Cancel", click: function() { cb(false); $( this ).dialog( "close" ); }	}
			],
			open: function(e) { RED.keyboard.disable();	},
			close: function(e) { RED.keyboard.enable();	}

		});
		$( "#node-dialog-verify-textTitle" ).text(textTitle);
		$( "#node-dialog-verify-text" ).html(text);
		$( "#node-dialog-verify" ).dialog('open');
	}


	$( "#node-dialog-confirm-deploy" ).dialog({
			title: "Confirm deploy",
			modal: true,
			autoOpen: false,
			width: 530,
			height: 230,
			buttons: [
				{
					text: "Confirm deploy",
					click: function() {
						save(true);
						$( this ).dialog( "close" );
					}
				},
				{
					text: "Cancel",
					click: function() {
						$( this ).dialog( "close" );
					}
				}
			]
	});

	// from http://css-tricks.com/snippets/javascript/get-url-variables/
	function getQueryVariable(variable) {
		var query = window.location.search.substring(1);
		var vars = query.split("&");
		for (var i=0;i<vars.length;i++) {
			var pair = vars[i].split("=");
			if(pair[0] == variable){return pair[1];}
		}
		return(false);
	}

	function loadNodes() {
			$(".palette-scroll").show();
			$("#palette-search").show();
			RED.storage.load();
			addClassTabsToPalette();
			refreshClassNodes();
			RED.view.redraw();
			
			setTimeout(function() {
				$("#btn-deploy").removeClass("disabled").addClass("btn-danger");
				$("#btn-deploy2").removeClass("disabled").addClass("btn-danger");
				$("#btn-import").removeClass("disabled").addClass("btn-success");
			}, 1500);
			//$('#btn-deploy').click(function() { save(); }); // duplicate
			//$('#btn-deploy2').click(function() { save2(); }); // duplicate
			// if the query string has ?info=className, populate info tab
			var info = getQueryVariable("info");
			if (info) {
				RED.sidebar.info.setHelpContent('', info);
			}
	}

	$('#btn-node-status').click(function() {toggleStatus();});

	var statusEnabled = false;
	function toggleStatus() {
		var btnStatus = $("#btn-node-status");
		statusEnabled = btnStatus.toggleClass("active").hasClass("active");
		RED.view.status(statusEnabled);
	}
	
	function showHelp() {

		var dialog = $('#node-help');

		//$("#node-help").draggable({
		//        handle: ".modal-header"
		//});

		dialog.on('show',function() {
			RED.keyboard.disable();
		});
		dialog.on('hidden',function() {
			RED.keyboard.enable();
		});

		dialog.modal();
	}

	$(function() {
		$(".palette-spinner").show();

		// server test switched off - test purposes only
		var patt = new RegExp(/^[http|https]/);
		var server = false && patt.test(location.protocol);

		if (!server) {
			
			var metaData = $.parseJSON($("script[data-container-name|='InputOutputCompatibilityMetadata']").html());
			requirements = metaData["requirements"];
			
			var data = $.parseJSON($("script[data-container-name|='NodeDefinitions']").html());
			var nodes = data["nodes"];
			$.each(nodes, function (key, val) {
				RED.nodes.registerType(val["type"], val["data"]);
			});
			RED.keyboard.add(/* ? */ 191, {shift: true}, function () {
				showHelp();
				d3.event.preventDefault();
			});
			loadNodes();
			$(".palette-spinner").hide();
		} else {
			$.ajaxSetup({beforeSend: function(xhr){
				if (xhr.overrideMimeType) {
					xhr.overrideMimeType("application/json");
				}
			}});
			$.getJSON( "resources/nodes_def.json", function( data ) {
				var nodes = data["nodes"];
				$.each(nodes, function(key, val) {
					RED.nodes.registerType(val["type"], val["data"]);
				});
				RED.keyboard.add(/* ? */ 191,{shift:true},function(){showHelp();d3.event.preventDefault();});
				loadNodes();
				$(".palette-spinner").hide();
			})
		}
	});

	return {
		addClassTabsToPalette:addClassTabsToPalette,
		refreshClassNodes:refreshClassNodes,
		classColor:classColor
	};
})();
