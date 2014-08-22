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

	$('#btn-keyboard-shortcuts').click(function(){showHelp();});

	function hideDropTarget() {
		$("#dropTarget").hide();
		RED.keyboard.remove(/* ESCAPE */ 27);
	}

	$('#chart').on("dragenter",function(event) {
		if ($.inArray("text/plain",event.originalEvent.dataTransfer.types) != -1) {
			$("#dropTarget").css({display:'table'});
			RED.keyboard.add(/* ESCAPE */ 27,hideDropTarget);
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
		RED.view.importNodes(data);
		event.preventDefault();
	});

	function save(force) {
		if (RED.view.dirty()) {

			if (!force) {
				var invalid = false;
				var unknownNodes = [];
				RED.nodes.eachNode(function(node) {
					invalid = invalid || !node.valid;
					if (node.type === "unknown") {
						if (unknownNodes.indexOf(node.name) == -1) {
							unknownNodes.push(node.name);
						}
						invalid = true;
					}
				});
				if (invalid) {
					if (unknownNodes.length > 0) {
						$( "#node-dialog-confirm-deploy-config" ).hide();
						$( "#node-dialog-confirm-deploy-unknown" ).show();
						var list = "<li>"+unknownNodes.join("</li><li>")+"</li>";
						$( "#node-dialog-confirm-deploy-unknown-list" ).html(list);
					} else {
						$( "#node-dialog-confirm-deploy-config" ).show();
						$( "#node-dialog-confirm-deploy-unknown" ).hide();
					}
					$( "#node-dialog-confirm-deploy" ).dialog( "open" );
					return;
				}
			}
			var nns = RED.nodes.createCompleteNodeSet();
			// sort by horizontal position, plus slight vertical position,
			// for well defined update order that follows signal flow
			nns.sort(function(a,b){ return (a.x + a.y/250) - (b.x + b.y/250); });
			//console.log(JSON.stringify(nns));

			var cpp = "// GUItool: begin automatically generated code\n";
			// generate code for all audio processing nodes
			for (var i=0; i<nns.length; i++) {
				var n = nns[i];
				var node = RED.nodes.node(n.id);
				if (node && (node.outputs > 0 || node._def.inputs > 0)) {
					cpp += n.type + " ";
					for (var j=n.type.length; j<24; j++) cpp += " ";
					cpp += n.id + "; ";
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
									if (j == 0 && parts[1] == 0 && src && src.outputs == 1 && dst && dst._def.inputs == 1) {
										cpp += n.id + ", " + parts[0];
									} else {
										cpp += n.id + ", " + j + ", " + parts[0] + ", " + parts[1];
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
			//mouse_mode = RED.state.EXPORT;
			$("#dialog-form").html($("script[data-template-name='export-clipboard-dialog']").html());
			$("#node-input-export").val(cpp);
			$("#node-input-export").focus(function() {
				var textarea = $(this);
				textarea.select();
				textarea.mouseup(function() {
					textarea.unbind("mouseup");
					return false;
				});
			});
			$( "#dialog" ).dialog("option","title","Export nodes to clipboard").dialog( "open" );
			$("#node-input-export").focus();

/*
			$("#btn-icn-deploy").removeClass('icon-upload');
			$("#btn-icn-deploy").addClass('spinner');
			RED.view.dirty(false);
			
			$.ajax({
				url:"flows",
				type: "POST",
				data: JSON.stringify(nns),
				contentType: "application/json; charset=utf-8"
			}).done(function(data,textStatus,xhr) {
				RED.notify("Successfully deployed","success");
				RED.nodes.eachNode(function(node) {
					if (node.changed) {
						node.dirty = true;
						node.changed = false;
					}
					if(node.credentials) {
						delete node.credentials;
					}
				});
				RED.nodes.eachConfig(function (confNode) {
					if (confNode.credentials) {
						delete confNode.credentials;
					}
				});
				// Once deployed, cannot undo back to a clean state
				RED.history.markAllDirty();
				RED.view.redraw();
			}).fail(function(xhr,textStatus,err) {
				RED.view.dirty(true);
				if (xhr.responseText) {
					RED.notify("<strong>Error</strong>: "+xhr.responseText,"error");
				} else {
					RED.notify("<strong>Error</strong>: no response from server","error");
				}
			}).always(function() {
				$("#btn-icn-deploy").removeClass('spinner');
				$("#btn-icn-deploy").addClass('icon-upload');
			});
*/
		}
	}

	$('#btn-deploy').click(function() { save(); });


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

	function loadSettings() {
/*
		$.get('settings', function(data) {
			RED.settings = data;
			console.log("Node-RED: "+data.version);
			loadNodes();
		});
*/
		loadNodes();
	}
	function loadNodes() {
	console.log("loadNodes");
		$.get('list.html', function(data) {
			console.log("loadNodes complete");
			$("body").append(data);
			$(".palette-spinner").hide();
			$(".palette-scroll").show();
			$("#palette-search").show();
			//loadFlows();
		}, "html");
	}

	function loadFlows() {
		$.getJSON("flows",function(nodes) {
			RED.nodes.import(nodes);
			RED.view.dirty(false);
			RED.view.redraw();
			RED.comms.subscribe("status/#",function(topic,msg) {
				var parts = topic.split("/");
				var node = RED.nodes.node(parts[1]);
				if (node) {
					node.status = msg;
					if (statusEnabled) {
						node.dirty = true;
						RED.view.redraw();
					}
				}
			});
			RED.comms.subscribe("node/#",function(topic,msg) {
				var i;
				if (topic == "node/added") {
					for (i=0;i<msg.length;i++) {
						var m = msg[i];
						var id = m.id;
						$.get('nodes/'+id, function(data) {
							$("body").append(data);
							var typeList = "<ul><li>"+m.types.join("</li><li>")+"</li></ul>";
							RED.notify("Node"+(m.types.length!=1 ? "s":"")+" added to palette:"+typeList,"success");
						});
					}
				} else if (topic == "node/removed") {
					if (msg.types) {
						for (i=0;i<msg.types.length;i++) {
							RED.palette.remove(msg.types[i]);
						}
						var typeList = "<ul><li>"+msg.types.join("</li><li>")+"</li></ul>";
						RED.notify("Node"+(msg.types.length!=1 ? "s":"")+" removed from palette:"+typeList,"success");
					}
				}
			});
		});
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
		RED.keyboard.add(/* ? */ 191,{shift:true},function(){showHelp();d3.event.preventDefault();});
		loadSettings();
		RED.comms.connect();
	});

	return {
	};
})();
