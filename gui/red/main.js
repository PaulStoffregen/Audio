/** Modified from original Node-Red source, for audio system visualization
 * //NOTE: code generation save function have moved to arduino-export.js
 *
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
/**
 * node RED namespace
 */
var RED = (function() { // this is used so that RED can be used as root "namespace"
	return {
		console_ok:function console_ok(text) { console.log('%c' + text, 'background: #ccffcc; color: #000'); }
	};
})();

/**
 * node RED main - here the main entry function exist
 */
RED.main = (function() {
	
	//NOTE: code generation save function have moved to arduino-export.js
	
	//var classColor = "#E6E0F8"; // standard
	var classColor = "#ccffcc"; // new
	var requirements;
	$('#btn-keyboard-shortcuts').click(function(){showHelp();});

	function hideDropTarget() {
		$("#dropTarget").hide();
		RED.keyboard.remove(/* ESCAPE */ 27);
	}

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
		RED.storage.loadContents(contents);
	}
	   
	document.getElementById('file-input').addEventListener('change', readSingleFile, false);
	
	$('#btn-saveTofile').click(function() { saveAsFile(); });
	function saveAsFile()
	{
		showSelectNameDialog();
	}
	
	function getConfirmLoadDemoText(filename)
	{
		return "<p> You are going to replace<br> <b>current flow</b>" +
			   " with <b>" + filename + "</b>.</p><br>" +
			   "<p>Are you sure you want to load?</p><br>" +
			   "<p>Note. your current design will be automatically downloaded as <b>TeensyAudioDesign.json</b></p><br>"+
			   "If you want a different filename,<br>then use the<b> export menu - SaveToFile</b> instead.";
	}
	function addDemoFlowsToMenu()
	{
		var html = "";
		
		html += '<li><a id="btn-demoFlowA" class="btn action-import enabled" href="#"><i id="btn-icn-download" class="icon-download"></i>DemoFlowA</a></li>';
		html += '<li><a id="btn-demoFlowB" class="btn action-import enabled" href="#"><i id="btn-icn-download" class="icon-download"></i>DemoFlowB</a></li>';
		html += '<li><a id="btn-originalFlow" class="btn action-import enabled" href="#"><i id="btn-icn-download" class="icon-download"></i>originalFlow</a></li>';
		html += '<li><a id="btn-emptyFlow" class="btn action-import enabled" href="#"><i id="btn-icn-download" class="icon-download"></i>Empty Flow</a></li>';
		$("#menu-demo-flows").next().append(html);
	
		$('#btn-demoFlowA').click(function() {
			var data = $("script[data-container-name|='DemoFlowA']").html();
			verifyDialog("Confirm Load", "!!!WARNING!!!", getConfirmLoadDemoText("DemoFlowA"), function(okPressed) { 
				if (okPressed)
				{
					console.error("load demo A");
					console.log("newFlowData:" + data);
					saveToFile("TeensyAudioDesign.json");
					RED.storage.loadContents(data);
				}
			});
		});
		$('#btn-demoFlowB').click(function() {
			var data = $("script[data-container-name|='DemoFlowB']").html();
			verifyDialog("Confirm Load", "!!!WARNING!!!", getConfirmLoadDemoText("DemoFlowB"), function(okPressed) { 
				if (okPressed)
				{
					console.warn("load demo B");
					console.log("newFlowData:" + data);
					saveToFile("TeensyAudioDesign.json");
					RED.storage.loadContents(data);
				}
			});
			
		});
		$('#btn-originalFlow').click(function() {
			var data = $("script[data-container-name|='FlowOriginal']").html();
			verifyDialog("Confirm Load", "!!!WARNING!!!", getConfirmLoadDemoText("FlowOriginal"), function(okPressed) { 
				if (okPressed)
				{
					console.warn("load demo original");
					console.log("newFlowData:" + data);
					saveToFile("TeensyAudioDesign.json");
					RED.storage.loadContents(data);
				}
			});
			
		});
		$('#btn-emptyFlow').click(function() {

			verifyDialog("Confirm Load", "!!!WARNING!!!", getConfirmLoadDemoText("FlowOriginal"), function(okPressed) { 
				if (okPressed)
				{
					console.warn("load empty flow")
					saveToFile("TeensyAudioDesign.json");
					RED.storage.loadContents(""); // [{"type":"tab","id":"Main","label":"Main","inputs":0,"outputs":0,"export":true,"nodes":[]}]
				}
			});
			
		});
	}
	
	// function save(force)
	//NOTE: code generation save function have moved to arduino-export.js
	
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

	
	function saveToFile(name)
	{
		try
		{
			var nns = RED.nodes.createCompleteNodeSet();
			var jsonString  = JSON.stringify(nns, null, 4);
			download(name, jsonString);
		}catch (err)
		{

		}
	}
	function showSelectNameDialog()
	{
		$( "#select-name-dialog" ).dialog({
			title: "Confirm deploy",
			modal: true,
			autoOpen: true,
			width: 530,
			height: 230,
			buttons: [
				{
					text: "Ok",
					click: function() {
						//console.warn($( "#select-name-dialog-name" ).val());
						saveToFile($( "#select-name-dialog-name" ).val());
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
		if ($( "#select-name-dialog-name" ).val().trim().length == 0)
			$( "#select-name-dialog-name" ).val("TeensyAudioDesign.json");
		$( "#select-name-dialog" ).dialog('open');
	}

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
			RED.nodes.addClassTabsToPalette();
			RED.nodes.refreshClassNodes();
			RED.view.redraw();
			
			setTimeout(function() {
				$("#menu-import").removeClass("disabled").addClass("btn-success");
				$("#menu-export").removeClass("disabled").addClass("btn-danger");
				$("#menu-arduino").removeClass("disabled").addClass("btn-warning");
			}, 1000);
			
			// if the query string has ?info=className, populate info tab
			var info = getQueryVariable("info");
			if (info) {
				RED.sidebar.info.setHelpContent('', info);
			}
	}

/*	$('#btn-node-status').click(function() {toggleStatus();});
	var statusEnabled = false;
	function toggleStatus() {
		var btnStatus = $("#btn-node-status");
		statusEnabled = btnStatus.toggleClass("active").hasClass("active");
		RED.view.status(statusEnabled);
	}
*/	
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
	function update(picker, selector) {
		document.querySelector(selector).style.background = picker.toBackground()
	}
	$(function()  // jQuery short-hand for $(document).ready(function() { ... });
	{	
		
		addDemoFlowsToMenu();
		RED.view.init();

		jscolor.presets.default = {
			closeButton:true
		};
		jscolor.trigger('input change');
		jscolor.installByClassName("jscolor");
		
		console.warn("main $(function() {...}); is executed after page load!"); // to see load order
		$(".palette-spinner").show();
		RED.settings.createTab();
		// server test switched off - test purposes only
		var patt = new RegExp(/^[http|https]/);
		var server = false && patt.test(location.protocol);

		if (!server) {
			
			var metaData = $.parseJSON($("script[data-container-name|='InputOutputCompatibilityMetadata']").html());
			// RED.main.requirements is needed because $(function() executes at global scope, 
			// if we just set requirements without RED.main. it's gonna be located in global scope
			// and in that case later we cannot use RED.main.requirements because that is unassigned.
			RED.main.requirements = metaData["requirements"]; // RED.main. is used to clarify the location of requirements
			
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
		classColor:classColor,
		requirements:requirements
	};
})();
