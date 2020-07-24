/** Modified from original Node-Red source, for audio system visualization
 * vim: set ts=4:
 * Copyright 2013, 2014 IBM Corp.
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


RED.view = (function() {
	var showGridVminor = true,
	    showGridVmajor = true,
		showGridH = true;
		
	var snapToGrid = true, // this is allready implemented with shift button, this locks that mode
	    snapToGridXsize = 5,
	    snapToGridYsize = 5;

	var showWorkspaceToolbar = true;

	var space_width = 5000,
		space_height = 5000,
		lineCurveScale = 0.75,
		scaleFactor = 1,
		node_width = 100,
		node_height = 35;

	var touchLongPressTimeout = 1000,
		startTouchDistance = 0,
		startTouchCenter = [],
		moveTouchCenter = [],
		touchStartTime = 0;

	var activeWorkspace = 0;
	var workspaceScrollPositions = {};

	var selected_link = null,
		mousedown_link = null,
		mousedown_node = null,
		mousedown_port_type = null,
		mousedown_port_index = 0,
		mouseup_node = null,
		mouse_offset = [0,0],
		mouse_position = null,
		mouse_mode = 0,
		moving_set = [], // the data type of this is a rect
		current_popup_rect = null,
		dirty = false,
		lasso = null,
		showStatus = false,
		lastClickNode = null,
		dblClickPrimed = null,
		clickTime = 0,
		clickElapsed = 0;

	var clipboard = "";

	var status_colours = {
		"red":    "#c00",
		"green":  "#5a8",
		"yellow": "#F9DF31",
		"blue":   "#53A3F3",
		"grey":   "#d3d3d3"
	};

	var outer = d3.select("#chart")
		.append("svg:svg")
		.attr("width", space_width)
		.attr("height", space_height)
		.attr("pointer-events", "all")
		.style("cursor","crosshair");

	 var vis = outer
		.append('svg:g')
		.on("dblclick.zoom", null)
		.append('svg:g')
		.on("mousemove", canvasMouseMove)
		.on("mousedown", canvasMouseDown)
		.on("mouseup", canvasMouseUp)
		.on("touchend", function() {
			clearTimeout(touchStartTime);
			touchStartTime = null;
			if  (RED.touch.radialMenu.active()) {
				return;
			}
			if (lasso) {
				outer_background.attr("fill","#fff");
			}
			canvasMouseUp.call(this);
		})
		.on("touchcancel", canvasMouseUp)
		.on("touchstart", function() {
			var touch0;
			if (d3.event.touches.length>1) {
				clearTimeout(touchStartTime);
				touchStartTime = null;
				d3.event.preventDefault();
				touch0 = d3.event.touches.item(0);
				var touch1 = d3.event.touches.item(1);
				var a = touch0['pageY']-touch1['pageY'];
				var b = touch0['pageX']-touch1['pageX'];

				var offset = $("#chart").offset();
				var scrollPos = [$("#chart").scrollLeft(),$("#chart").scrollTop()];
				startTouchCenter = [
					(touch1['pageX']+(b/2)-offset.left+scrollPos[0])/scaleFactor,
					(touch1['pageY']+(a/2)-offset.top+scrollPos[1])/scaleFactor
				];
				moveTouchCenter = [
					touch1['pageX']+(b/2),
					touch1['pageY']+(a/2)
				];
				startTouchDistance = Math.sqrt((a*a)+(b*b));
			} else {
				var obj = d3.select(document.body);
				touch0 = d3.event.touches.item(0);
				var pos = [touch0.pageX,touch0.pageY];
				startTouchCenter = [touch0.pageX,touch0.pageY];
				startTouchDistance = 0;
				var point = d3.touches(this)[0];
				touchStartTime = setTimeout(function() {
					touchStartTime = null;
					showTouchMenu(obj,pos);
					//lasso = vis.append('rect')
					//    .attr("ox",point[0])
					//    .attr("oy",point[1])
					//    .attr("rx",2)
					//    .attr("ry",2)
					//    .attr("x",point[0])
					//    .attr("y",point[1])
					//    .attr("width",0)
					//    .attr("height",0)
					//    .attr("class","lasso");
					//outer_background.attr("fill","#e3e3f3");
				},touchLongPressTimeout);
			}
		})
		.on("touchmove", function(){
				if  (RED.touch.radialMenu.active()) {
					d3.event.preventDefault();
					return;
				}
				var touch0;
				if (d3.event.touches.length<2) {
					if (touchStartTime) {
						touch0 = d3.event.touches.item(0);
						var dx = (touch0.pageX-startTouchCenter[0]);
						var dy = (touch0.pageY-startTouchCenter[1]);
						var d = Math.abs(dx*dx+dy*dy);
						if (d > 64) {
							clearTimeout(touchStartTime);
							touchStartTime = null;
						}
					} else if (lasso) {
						d3.event.preventDefault();
					}
					canvasMouseMove.call(this);
				} else {
					touch0 = d3.event.touches.item(0);
					var touch1 = d3.event.touches.item(1);
					var a = touch0['pageY']-touch1['pageY'];
					var b = touch0['pageX']-touch1['pageX'];
					var offset = $("#chart").offset();
					var scrollPos = [$("#chart").scrollLeft(),$("#chart").scrollTop()];
					var moveTouchDistance = Math.sqrt((a*a)+(b*b));
					var touchCenter = [
						touch1['pageX']+(b/2),
						touch1['pageY']+(a/2)
					];

					if (!isNaN(moveTouchDistance)) {
						oldScaleFactor = scaleFactor;
						scaleFactor = Math.min(2,Math.max(0.3, scaleFactor + (Math.floor(((moveTouchDistance*100)-(startTouchDistance*100)))/10000)));

						var deltaTouchCenter = [                             // Try to pan whilst zooming - not 100%
							startTouchCenter[0]*(scaleFactor-oldScaleFactor),//-(touchCenter[0]-moveTouchCenter[0]),
							startTouchCenter[1]*(scaleFactor-oldScaleFactor) //-(touchCenter[1]-moveTouchCenter[1])
						];

						startTouchDistance = moveTouchDistance;
						moveTouchCenter = touchCenter;

						$("#chart").scrollLeft(scrollPos[0]+deltaTouchCenter[0]);
						$("#chart").scrollTop(scrollPos[1]+deltaTouchCenter[1]);
						redraw();
					}
				}
		});

	var outer_background = vis.append('svg:rect')
		.attr('width', space_width)
		.attr('height', space_height)
		.attr('fill','#fff');

	var gridScale = d3.scale.linear().range([0,5000]).domain([0,5000]);
	var gridH = vis.append('g').attr({
		"id":"grid-h",
		"style":"display:none;"
		});
	var gridVminor = vis.append('g').attr({ // minor gets rendered first
		"id":"grid-v-mi",
		"style":"display:none;"
	});
	var gridVmajor = vis.append('g').attr({ // major then "overrides" the minors
		"id":"grid-v-ma",
		"style":"display:none;"
		});
	
	initGrid();
	var drag_line = vis.append("svg:path").attr("class", "drag_line");
	
	
	function initGrid()
	{
		gridH.selectAll("line.horizontal").data(gridScale.ticks(250)).enter()
	    	.append("line")
	        .attr(
	        {
	            "class":"horizontal",
	            "x1" : 0,
	            "x2" : 5000,
	            "y1" : function(d){ return gridScale(d);},
	            "y2" : function(d){ return gridScale(d);},
	            "fill" : "none",
	            "shape-rendering" : "optimizeSpeed",
	            "stroke" : "#eee",
				//"stroke-dasharray":"2",
	            "stroke-width" : "1px"
			});
		gridVminor.selectAll("line.vertical").data(gridScale.ticks(250)).enter()
	     	.append("line")
	        .attr(
	        {
	            "class":"vertical",
	            "y1" : 0,
	            "y2" : 5000,
	            "x1" : function(d){ return gridScale(d);},
	            "x2" : function(d){ return gridScale(d);},
	            "fill" : "none",
	            "shape-rendering" : "optimizeSpeed",
		        "stroke" : "#eee",
				//"stroke-dasharray":"2",
	            "stroke-width" : "1px"
			});
		gridVmajor.selectAll("line.vertical").data(gridScale.ticks(50)).enter()
	     	.append("line")
	        .attr(
	        {
	            "class":"vertical",
	            "y1" : 0,
	            "y2" : 5000,
	            "x1" : function(d){ return gridScale(d);},
	            "x2" : function(d){ return gridScale(d);},
	            "fill" : "none",
	            "shape-rendering" : "optimizeSpeed",
		        "stroke" : "#aaa",
				//"stroke-dasharray":"2",
	            "stroke-width" : "2px"
			});
		
			// sets visibility according to vars, so that it's only changed at one place
			showHideGridH(showGridH);
			showHideGridVmajor(showGridVmajor);
			showHideGridVminor(showGridVminor);
	}
	function setSnapToGrid(state)
	{
		snapToGrid = state;
	}
	function showHideGrid(state)
	{
		showHideGridH(showGridH || state);
		showHideGridVmajor(showGridVmajor || state);
		showHideGridVminor(showGridVminor || state);
	}

	function showHideGridVmajor(state)
	{
		if (state == true) {
			$('#grid-v-ma').attr("style", "display:block;");
		} else {
			$('#grid-v-ma').attr("style", "display:none;");
		}
	}
	function showHideGridVminor(state)
	{
		if (state == true) {
			$('#grid-v-mi').attr("style", "display:block;");
		} else {
			$('#grid-v-mi').attr("style", "display:none;");
		}
	}
	function showHideGridH(state)
	{
		if (state == true) {
			$('#grid-h').attr("style", "display:block;");
		} else {
			$('#grid-h').attr("style", "display:none;");
		}
	}

	$('#btn-cut').click(function() {copySelection();deleteSelection();});
	$('#btn-copy').click(function() {copySelection()});
	$('#btn-paste').click(function() {importNodes(clipboard)});

	var workspace_tabs = RED.tabs.create({
		id: "workspace-tabs",
		onchange: function(tab) {
			setShowWorkspaceToolbarVisible(showWorkspaceToolbar);
						
			console.log("workspace_tabs onchange:" + tab);
			var chart = $("#chart");
			if (activeWorkspace !== 0) {
				workspaceScrollPositions[activeWorkspace] = {
					left:chart.scrollLeft(),
					top:chart.scrollTop()
				};
			}
			var scrollStartLeft = chart.scrollLeft();
			var scrollStartTop = chart.scrollTop();

			activeWorkspace = tab.id;
			if (workspaceScrollPositions[activeWorkspace]) {
				chart.scrollLeft(workspaceScrollPositions[activeWorkspace].left);
				chart.scrollTop(workspaceScrollPositions[activeWorkspace].top);
			} else {
				chart.scrollLeft(0);
				chart.scrollTop(0);
			}
			var scrollDeltaLeft = chart.scrollLeft() - scrollStartLeft;
			var scrollDeltaTop = chart.scrollTop() - scrollStartTop;
			if (mouse_position != null) {
				mouse_position[0] += scrollDeltaLeft;
				mouse_position[1] += scrollDeltaTop;
			}

			clearSelection();
			RED.nodes.eachNode(function(n) {
					n.dirty = true;
			});
			redraw();
		},
		ondblclick: function(tab) {
			showRenameWorkspaceDialog(tab.id);
		},
		onadd: function(tab) { // this is a callback called from tabs.js
			var menuli = $("<li/>");
			var menuA = $("<a/>",{tabindex:"-1",href:"#"+tab.id}).appendTo(menuli);
			menuA.html(tab.label);
			menuA.on("click",function() {
				workspace_tabs.activateTab(tab.id);
			});

			$('#workspace-menu-list').append(menuli);

			if (workspace_tabs.count() == 1) {
				$('#btn-workspace-delete').parent().addClass("disabled");
			} else {
				$('#btn-workspace-delete').parent().removeClass("disabled");
			}

			var link = $("#workspace-tabs a[href='#"+tab.id+"']");
			if (!tab.export)
				link.attr("style", "color:#b3b3b3;");
			else // 
				link.attr("style", "color:#000000;");
		},
		onremove: function(tab) {
			if (workspace_tabs.count() == 1) {
				$('#btn-workspace-delete').parent().addClass("disabled");
			} else {
				$('#btn-workspace-delete').parent().removeClass("disabled");
			}
			$('#workspace-menu-list a[href="#'+tab.id+'"]').parent().remove();

		}
	});

	var workspaceIndex = 0;

	function addWorkspace() {
		var tabId = RED.nodes.id();
		do {
			workspaceIndex += 1;
		} while($("#workspace-tabs a[title='Sheet_"+workspaceIndex+"']").size() !== 0);

		var ws = RED.nodes.createWorkspaceObject(tabId, "Sheet_"+workspaceIndex, 0, 0, true);
		RED.nodes.addWorkspace(ws);
		workspace_tabs.addTab(ws);
		workspace_tabs.activateTab(tabId);
		RED.history.push({t:'add',workspaces:[ws],dirty:dirty});
		RED.view.dirty(true);
	}
	$('#btn-workspace-add-tab').on("click",addWorkspace);
	$('#btn-workspace-add').on("click",addWorkspace);
	$('#btn-workspace-edit').on("click",function() {
		showRenameWorkspaceDialog(activeWorkspace);
	});
	$('#btn-workspace-delete').on("click",function() {
		deleteWorkspace(activeWorkspace);
	});

	function deleteWorkspace(id) {
		if (workspace_tabs.count() == 1) {
			return;
		}
		var ws = RED.nodes.workspace(id);
		$( "#node-dialog-delete-workspace" ).dialog('option','workspace',ws);
		$( "#node-dialog-delete-workspace-name" ).text(ws.label);
		$( "#node-dialog-delete-workspace" ).dialog('open');
	}

	function canvasMouseDown() {
		if (!mousedown_node && !mousedown_link) {
			selected_link = null;
			updateSelection();
		}
		if (mouse_mode === 0) {
			if (lasso) {
				lasso.remove();
				lasso = null;
			}

			if (!touchStartTime) {
				var point = d3.mouse(this);
				lasso = vis.append('rect')
					.attr("ox",point[0])
					.attr("oy",point[1])
					.attr("rx",2)
					.attr("ry",2)
					.attr("x",point[0])
					.attr("y",point[1])
					.attr("width",0)
					.attr("height",0)
					.attr("class","lasso");
				d3.event.preventDefault();
			}
		}
	}

	function canvasMouseMove() {
		mouse_position = d3.touches(this)[0]||d3.mouse(this);

		// Prevent touch scrolling...
		//if (d3.touches(this)[0]) {
		//    d3.event.preventDefault();
		//}
		var chart = $("#chart");
		
		// TODO: auto scroll the container
		//var point = d3.mouse(this);
		//if (point[0]-container.scrollLeft < 30 && container.scrollLeft > 0) { container.scrollLeft -= 15; }
		//console.log(d3.mouse(this));//,container.offsetWidth,container.offsetHeight,container.scrollLeft,container.scrollTop);

		if (lasso) {
			
			var ox = parseInt(lasso.attr("ox"));
			var oy = parseInt(lasso.attr("oy"));
			var x = parseInt(lasso.attr("x"));
			var y = parseInt(lasso.attr("y"));
			
			var w;
			var h;
			if (mouse_position[0] < ox) {
				x = mouse_position[0];
				w = ox-x;
			} else {
				w = mouse_position[0]-x;
			}
			if (mouse_position[1] < oy) {
				y = mouse_position[1];
				h = oy-y;
			} else {
				h = mouse_position[1]-y;
			}
			lasso
				.attr("x",x)
				.attr("y",y)
				.attr("width",w)
				.attr("height",h)
			;
			return;
		}

		if (mouse_mode != RED.state.IMPORT_DRAGGING && !mousedown_node && selected_link == null) {
			return;
		}

		var mousePos;
		if (mouse_mode == RED.state.JOINING) {
			// update drag line
			drag_line.attr("class", "drag_line");
			mousePos = mouse_position;
			
			var numOutputs = 0;
			if (mousedown_node.inputs) numOutputs = (mousedown_port_type === 0)?(mousedown_node.outputs || 1):(mousedown_node.inputs || 1); // Jannik
			else numOutputs = (mousedown_port_type === 0)?(mousedown_node.outputs || 1):(mousedown_node._def.inputs || 1);

			var sourcePort = mousedown_port_index;
			var portY = -((numOutputs-1)/2)*13 +13*sourcePort;

			var sc = (mousedown_port_type === 0)?1:-1;

			var dy = mousePos[1]-(mousedown_node.y+portY);
			var dx = mousePos[0]-(mousedown_node.x+sc*mousedown_node.w/2);
			var delta = Math.sqrt(dy*dy+dx*dx);
			var scale = lineCurveScale;
			var scaleY = 0;

			if (delta < node_width) {
				scale = 0.75-0.75*((node_width-delta)/node_width);
			}
			if (dx*sc < 0) {
				scale += 2*(Math.min(5*node_width,Math.abs(dx))/(5*node_width));
				if (Math.abs(dy) < 3*node_height) {
					scaleY = ((dy>0)?0.5:-0.5)*(((3*node_height)-Math.abs(dy))/(3*node_height))*(Math.min(node_width,Math.abs(dx))/(node_width)) ;
				}
			}

			drag_line.attr("d",
				"M "+(mousedown_node.x+sc*mousedown_node.w/2)+" "+(mousedown_node.y+portY)+
				" C "+(mousedown_node.x+sc*(mousedown_node.w/2+node_width*scale))+" "+(mousedown_node.y+portY+scaleY*node_height)+" "+
				(mousePos[0]-sc*(scale)*node_width)+" "+(mousePos[1]-scaleY*node_height)+" "+
				mousePos[0]+" "+mousePos[1]
				);
			d3.event.preventDefault();
		} else if (mouse_mode == RED.state.MOVING) {
			mousePos = mouse_position;
			var d = (mouse_offset[0]-mousePos[0])*(mouse_offset[0]-mousePos[0]) + (mouse_offset[1]-mousePos[1])*(mouse_offset[1]-mousePos[1]);
			if (d > 2) {
				mouse_mode = RED.state.MOVING_ACTIVE;
				clickElapsed = 0;
			}
		} else if (mouse_mode == RED.state.MOVING_ACTIVE || mouse_mode == RED.state.IMPORT_DRAGGING) {
			mousePos = mouse_position;
			var node;
			var i;
			var minX = 0;
			var minY = 0;
			for (var n = 0; n<moving_set.length; n++) {
				node = moving_set[n];
				if (d3.event.shiftKey) {
					node.n.ox = node.n.x;
					node.n.oy = node.n.y;
				}
				node.n.x = mousePos[0]+node.dx;
				node.n.y = mousePos[1]+node.dy;
				node.n.dirty = true;
				minX = Math.min(node.n.x-node.n.w/2-5,minX);
				minY = Math.min(node.n.y-node.n.h/2-5,minY);
			}
			if (minX !== 0 || minY !== 0) {
				for (i = 0; i<moving_set.length; i++) {
					node = moving_set[i];
					node.n.x -= minX;
					node.n.y -= minY;
				}
			}
			// snap to grid
			if (((snapToGrid == true) || d3.event.shiftKey) && moving_set.length > 0) {
				var gridOffset =  [0,0];
				node = moving_set[0];
				//gridOffset[0] = node.n.x-(snapToGridXsize*Math.floor((node.n.x-node.n.w/2)/snapToGridXsize)+node.n.w/2);
				gridOffset[0] = node.n.x-(snapToGridXsize*Math.floor(node.n.x/snapToGridXsize)); // this works much better than above
				gridOffset[1] = node.n.y-(snapToGridYsize*Math.floor(node.n.y/snapToGridYsize));
				if (gridOffset[0] !== 0 || gridOffset[1] !== 0) {
					for (i = 0; i<moving_set.length; i++) {
						node = moving_set[i];
						node.n.x -= gridOffset[0];
						node.n.y -= gridOffset[1];
						if (node.n.x == node.n.ox && node.n.y == node.n.oy) {
							node.dirty = false;
						}
					}
				}
			}
		}
		redraw();
	}

	function canvasMouseUp() {
		if (mousedown_node && mouse_mode == RED.state.JOINING) {
			drag_line.attr("class", "drag_line_hidden");
		}
		if (lasso) {
			console.warn("canvasMouseUp lasso happend");
			var x = parseInt(lasso.attr("x"));
			var y = parseInt(lasso.attr("y"));
			var x2 = x+parseInt(lasso.attr("width"));
			var y2 = y+parseInt(lasso.attr("height"));
			if (!d3.event.ctrlKey) {
				clearSelection();
			}
			RED.nodes.eachNode(function(n) {
				if (n.z == activeWorkspace && !n.selected) {
					n.selected = (n.x > x && n.x < x2 && n.y > y && n.y < y2);
					if (n.selected) {
						n.dirty = true;
						moving_set.push({n:n});
					}
				}
			});
			updateSelection();
			lasso.remove();
			lasso = null;
		} else if (mouse_mode == RED.state.DEFAULT && mousedown_link == null) {
			console.warn("canvasMouseUp mouse_mode == RED.state.DEFAULT && mousedown_link == null) happend");
			clearSelection();
			updateSelection();
		}
		if (mouse_mode == RED.state.MOVING_ACTIVE) {
			console.warn("canvasMouseUp mouse_mode == RED.state.MOVING_ACTIVE happend");
			if (moving_set.length > 0) {
				var ns = [];
				for (var j=0;j<moving_set.length;j++) {
					ns.push({n:moving_set[j].n,ox:moving_set[j].ox,oy:moving_set[j].oy});
				}
				RED.history.push({t:'move',nodes:ns,dirty:dirty});
				RED.storage.update();
			}
		}
		if (mouse_mode == RED.state.MOVING || mouse_mode == RED.state.MOVING_ACTIVE) {
			for (var i=0;i<moving_set.length;i++) {
				delete moving_set[i].ox;
				delete moving_set[i].oy;
			}
		}
		if (mouse_mode == RED.state.IMPORT_DRAGGING) {
			RED.keyboard.remove(/* ESCAPE */ 27);
			setDirty(true);
		}
		redraw();
		// clear mouse event vars
		resetMouseVars();
	}

	$('#btn-zoom-out').click(function() {zoomOut();});
	$('#btn-zoom-zero').click(function() {zoomZero();});
	$('#btn-zoom-in').click(function() {zoomIn();});
	$("#chart").on('DOMMouseScroll mousewheel', function (evt) {
		if ( evt.altKey ) {
			evt.preventDefault();
			evt.stopPropagation();
			var move = -(evt.originalEvent.detail) || evt.originalEvent.wheelDelta;
			if (move <= 0) { zoomOut(); }
			else { zoomIn(); }
		}
	});
	$("#chart").droppable({
			accept:".palette_node",
			drop: function( event, ui ) {
				d3.event = event;
				var selected_tool = ui.draggable[0].type;

				var mousePos = d3.touches(this)[0]||d3.mouse(this);
				mousePos[1] += this.scrollTop;
				mousePos[0] += this.scrollLeft;
				mousePos[1] /= scaleFactor;
				mousePos[0] /= scaleFactor;

				var nn = {x: mousePos[0],y:mousePos[1],w:node_width,z:activeWorkspace};
				nn.type = selected_tool;
				nn._def = RED.nodes.getType(nn.type);
				
				nn.id = RED.nodes.cppId(nn, RED.nodes.getWorkspace(activeWorkspace).label);  // jannik add/change
				nn.name = (nn._def.shortName) ? nn._def.shortName : nn.type.replace(/^Analog/, "");// jannik add/change temp name
				nn.name = RED.nodes.cppName(nn); // jannik add/change create unique name

				nn._def.defaults = nn._def.defaults ? nn._def.defaults  : {};
				nn._def.defaults.name = { value: nn.id };

				nn.outputs = nn._def.outputs;
				nn.changed = true;
				console.log("drop happend:" + selected_tool + ", type:" + nn.type);
				for (var d in nn._def.defaults) {
					if (nn._def.defaults.hasOwnProperty(d)) {
						if (d == "name" || d == "id") continue; // jannik add (this prevent above assigment to be overwritten)
						nn[d] = nn._def.defaults[d].value;
					}
				}

				if (nn._def.onadd) {
					nn._def.onadd.call(nn);
				}

				nn.h = Math.max(node_height,(nn.outputs||0) * 15);
				RED.history.push({t:'add',nodes:[nn.id],dirty:dirty});
				RED.nodes.add(nn);
				RED.editor.validateNode(nn);
				setDirty(true);
				// auto select dropped node - so info shows (if visible)
				clearSelection();
				nn.selected = true;
				moving_set.push({n:nn});
				updateSelection();
				redraw();
				if (nn._def.autoedit) {
					RED.editor.edit(nn);
				}
			}
	});

	function zoomIn() {
		if (scaleFactor < 0.3)
		{
			scaleFactor += 0.05;
			redraw();
		}
		else if (scaleFactor < 2) {
			scaleFactor += 0.1;
			redraw();
		}

	}
	function zoomOut() {
		if (scaleFactor > 0.3) {
			scaleFactor -= 0.1;
			redraw();
		}
		else if (scaleFactor > 0.1) {
			scaleFactor -= 0.05;
			redraw();
		}
	}
	function zoomZero() {
		scaleFactor = 1;
		redraw();
	}

	function selectAll() {
		RED.nodes.eachNode(function(n) {
			if (n.z == activeWorkspace) {
				if (!n.selected) {
					n.selected = true;
					n.dirty = true;
					moving_set.push({n:n});
				}
			}
		});
		selected_link = null;
		updateSelection();
		redraw();
	}

	function clearSelection() {
		for (var i=0;i<moving_set.length;i++) {
			var n = moving_set[i];
			n.n.dirty = true;
			n.n.selected = false;
		}
		moving_set = [];
		selected_link = null;
	}

	function updateSelection() {
		if (moving_set.length === 0) {
			$("#li-menu-export").addClass("disabled");
			$("#li-menu-export-clipboard").addClass("disabled");
			$("#li-menu-export-library").addClass("disabled");
		} else {
			$("#li-menu-export").removeClass("disabled");
			$("#li-menu-export-clipboard").removeClass("disabled");
			$("#li-menu-export-library").removeClass("disabled");
		}
		if (moving_set.length === 0 && selected_link == null) {
			RED.keyboard.remove(/* backspace */ 8);
			RED.keyboard.remove(/* delete */ 46);
			RED.keyboard.remove(/* c */ 67);
			RED.keyboard.remove(/* x */ 88);
		} else {
			//RED.keyboard.add(/* backspace */ 8,function(){deleteSelection();d3.event.preventDefault();}); // jannik thinks this is unlogical and unnecessary
			RED.keyboard.add(/* delete */ 46,function(){deleteSelection();d3.event.preventDefault();});
			RED.keyboard.add(/* c */ 67,{ctrl:true},function(){copySelection();d3.event.preventDefault();});
			RED.keyboard.add(/* x */ 88,{ctrl:true},function(){copySelection();deleteSelection();d3.event.preventDefault();});
		}
		if (moving_set.length === 0) {
			RED.keyboard.remove(/* up   */ 38);
			RED.keyboard.remove(/* down */ 40);
			RED.keyboard.remove(/* left */ 37);
			RED.keyboard.remove(/* right*/ 39);
		} else {
			RED.keyboard.add(/* up   */ 38, function() { if(d3.event.shiftKey || (snapToGrid == true)){moveSelection(  0,-snapToGridYsize)}else{moveSelection( 0,-1);}d3.event.preventDefault();},endKeyboardMove);
			RED.keyboard.add(/* down */ 40, function() { if(d3.event.shiftKey || (snapToGrid == true)){moveSelection(  0, snapToGridYsize)}else{moveSelection( 0, 1);}d3.event.preventDefault();},endKeyboardMove);
			RED.keyboard.add(/* left */ 37, function() { if(d3.event.shiftKey || (snapToGrid == true)){moveSelection(-snapToGridXsize,  0)}else{moveSelection(-1, 0);}d3.event.preventDefault();},endKeyboardMove);
			RED.keyboard.add(/* right*/ 39, function() { if(d3.event.shiftKey || (snapToGrid == true)){moveSelection( snapToGridXsize,  0)}else{moveSelection( 1, 0);}d3.event.preventDefault();},endKeyboardMove);
		}
		if (moving_set.length == 1) {
			RED.sidebar.info.refresh(moving_set[0].n);
			RED.nodes.selectNode(moving_set[0].n.type);
		} else if (moving_set.length > 1) {
			RED.sidebar.info.showSelection(moving_set);
		} else {
			RED.sidebar.info.clear();
		}
	}
	function endKeyboardMove() {
		var ns = [];
		for (var i=0;i<moving_set.length;i++) {
			ns.push({n:moving_set[i].n,ox:moving_set[i].ox,oy:moving_set[i].oy});
			delete moving_set[i].ox;
			delete moving_set[i].oy;
		}
		RED.history.push({t:'move',nodes:ns,dirty:dirty});// TODO: is this nessesary
	}
	function moveSelection(dx,dy) {
		var minX = 0;
		var minY = 0;
		var node;

		for (var i=0;i<moving_set.length;i++) {
			node = moving_set[i];
			if (node.ox == null && node.oy == null) {
				node.ox = node.n.x;
				node.oy = node.n.y;
			}
			node.n.x += dx;
			node.n.y += dy;
			node.n.dirty = true;
			minX = Math.min(node.n.x-node.n.w/2-5,minX);
			minY = Math.min(node.n.y-node.n.h/2-5,minY);
		}

		if (minX !== 0 || minY !== 0) {
			for (var n = 0; n<moving_set.length; n++) {
				node = moving_set[n];
				node.n.x -= minX;
				node.n.y -= minY;
			}
		}

		redraw();
	}
	function deleteSelection() {
		var removedNodes = [];
		var removedLinks = [];
		var startDirty = dirty;
		if (current_popup_rect)
			$(current_popup_rect).popover("destroy");
		if (moving_set.length > 0) {
			for (var i=0;i<moving_set.length;i++) {
				var node = moving_set[i].n; // moving_set[i] is a rect
				node.selected = false;
				if (node.x < 0) {
					node.x = 25
				}
				/*for (var ipi = 0; ipi < node.inputlist.length; ipi++)
				{
					$(node.inputlist[ipi]).popover("destroy"); // remove any popovers visible (otherwise they will be left behind)
				}
				for (var opi = 0; opi < node._ports.length; opi++)
				{
					$(node._ports[opi]).popover("destroy"); // remove any popovers visible (otherwise they will be left behind)
				}*/
				
				var rmlinks = RED.nodes.remove(node.id);
				for (var j=0; j < rmlinks.length; j++) {
					var link = rmlinks[j];
					//console.log("delete link: " + link.source.id + ":" + link.sourcePort
					//	+ " -> " + link.target.id + ":" + link.targetPort);
					if (link.source == node) {
						// reenable input port
						var n = link.targetPort;
						var rect = link.target.inputlist[n];
						rect.on("mousedown", (function(d,n){return function(d){portMouseDown(d,1,n);}})(rect, n))
							.on("touchstart", (function(d,n){return function(d){portMouseDown(d,1,n);}})(rect, n))
							.on("mouseup", (function(d,n){return function(d){portMouseUp(d,1,n);}})(rect, n))
							.on("touchend", (function(d,n){return function(d){portMouseUp(d,1,n);}})(rect, n))
							.on("mouseover", nodeInput_mouseover)
							.on("mouseout", nodePort_mouseout)
							//.on("mouseover",function(d) { var port = d3.select(this); port.classed("port_hovered",(mouse_mode!=RED.state.JOINING || mousedown_port_type != 1 ));})
							//.on("mouseout",function(d) { var port = d3.select(this); port.classed("port_hovered",false);})
					}
				}
				removedNodes.push(node);
				removedLinks = removedLinks.concat(rmlinks);
			}
			moving_set = [];
			setDirty(true);
		}
		if (selected_link) {
			// reenable input port
			var n = selected_link.targetPort;
			var rect = selected_link.target.inputlist[n];
			rect.on("mousedown", (function(d,n){return function(d){portMouseDown(d,1,n);}})(rect, n))
				.on("touchstart", (function(d,n){return function(d){portMouseDown(d,1,n);}})(rect, n))
				.on("mouseup", (function(d,n){return function(d){portMouseUp(d,1,n);}})(rect, n))
				.on("touchend", (function(d,n){return function(d){portMouseUp(d,1,n);}})(rect, n))
				.on("mouseover", nodeInput_mouseover)
				.on("mouseout", nodePort_mouseout)
				//.on("mouseover",function(d) { var port = d3.select(this); port.classed("port_hovered",(mouse_mode!=RED.state.JOINING || mousedown_port_type != 1 ));})
				//.on("mouseout",function(d) { var port = d3.select(this); port.classed("port_hovered",false);});
			RED.nodes.removeLink(selected_link);
			removedLinks.push(selected_link);
			setDirty(true);
		}
		RED.history.push({t:'delete',nodes:removedNodes,links:removedLinks,dirty:startDirty});

		selected_link = null;
		updateSelection();
		redraw();
	}

	function copySelection() {
		if (moving_set.length > 0) {
			var nns = [];
			for (var n=0;n<moving_set.length;n++) {
				var node = moving_set[n].n;
				nns.push(RED.nodes.convertNode(node));
			}
			clipboard = JSON.stringify(nns);
			RED.notify(moving_set.length+" node"+(moving_set.length>1?"s":"")+" copied");
		}
	}

	function calculateTextWidth(str) {
		var sp = document.createElement("span");
		sp.className = "node_label";
		sp.style.position = "absolute";
		sp.style.top = "-1000px";
		sp.innerHTML = (str||"").replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;");
		document.body.appendChild(sp);
		var w = sp.offsetWidth;
		document.body.removeChild(sp);
		return 50+w;
	}

	function resetMouseVars() {
		mousedown_node = null;
		mouseup_node = null;
		mousedown_link = null;
		mouse_mode = 0;
		mousedown_port_type = 0;
	}

	function portMouseDown(d,portType,portIndex) {
		// disable zoom
		//vis.call(d3.behavior.zoom().on("zoom"), null);
		mousedown_node = d;
		selected_link = null;
		mouse_mode = RED.state.JOINING;
		mousedown_port_type = portType;
		mousedown_port_index = portIndex || 0;
		document.body.style.cursor = "crosshair";
		d3.event.preventDefault();
	}

	function portMouseUp(d,portType,portIndex) {
		document.body.style.cursor = "";
		if (mouse_mode == RED.state.JOINING && mousedown_node) {
			if (typeof TouchEvent != "undefined" && d3.event instanceof TouchEvent) {
				RED.nodes.eachNode(function(n) {
						if (n.z == activeWorkspace) {
							var hw = n.w/2;
							var hh = n.h/2;
							if (n.x-hw<mouse_position[0] && n.x+hw> mouse_position[0] &&
								n.y-hh<mouse_position[1] && n.y+hh>mouse_position[1]) {
									mouseup_node = n;
									
									if (mouseup_node.inputs) portType = mouseup_node.inputs>0?1:0; // Jannik add so that input count can be changed on the fly
									else portType = mouseup_node._def.inputs>0?1:0;
									
									portIndex = 0;
							}
						}
				});
			} else {
				mouseup_node = d;
			}
			if (portType == mousedown_port_type || mouseup_node === mousedown_node) {
				drag_line.attr("class", "drag_line_hidden");
				resetMouseVars();
				return;
			}
			var src,dst,src_port,dst_port;
			if (mousedown_port_type === 0) {
				src = mousedown_node;
				src_port = mousedown_port_index;
				dst = mouseup_node;
				dst_port = portIndex;
			} else if (mousedown_port_type == 1) {
				src = mouseup_node;
				src_port = portIndex;
				dst = mousedown_node;
				dst_port = mousedown_port_index;
			}

			var existingLink = false;
			RED.nodes.eachLink(function(d) {
					existingLink = existingLink || (d.source === src && d.target === dst && d.sourcePort == src_port && d.targetPort == dst_port);
			});
			if (!existingLink) {
				var link = {source: src, sourcePort:src_port, target: dst, targetPort: dst_port};
				RED.nodes.addLink(link);
				RED.history.push({t:'add',links:[link],dirty:dirty});
				setDirty(true);
				// disallow new links to this destination - each input can have only a single link
				dst.inputlist[dst_port]
					.classed("port_hovered",false)
					.on("mousedown",null)
					.on("touchstart", null)
					.on("mouseup", null)
					.on("touchend", null)
					.on("mouseover", nodeInput_mouseover)
					.on("mouseout", nodePort_mouseout);
			}
			selected_link = null;
			redraw();
		}
	}

	function nodeMouseUp(d) {
		showHideGrid(false);
		if (dblClickPrimed && mousedown_node == d && clickElapsed > 0 && clickElapsed < 750) {
			RED.editor.edit(d);
			clickElapsed = 0;
			d3.event.stopPropagation();
			return;
		}
		if (d.inputs) portMouseUp(d, d.inputs > 0 ? 1 : 0, 0); // Jannik add so that input count can be changed on the fly
		else portMouseUp(d, d._def.inputs > 0 ? 1 : 0, 0);
	}

	function nodeMouseDown(d) {
		showHideGrid(true);
		//var touch0 = d3.event;
		//var pos = [touch0.pageX,touch0.pageY];
		//RED.touch.radialMenu.show(d3.select(this),pos);
		if (mouse_mode == RED.state.IMPORT_DRAGGING) {
			RED.keyboard.remove(/* ESCAPE */ 27);
			updateSelection();
			setDirty(true);
			redraw();
			resetMouseVars();
			d3.event.stopPropagation();
			return;
		}
		mousedown_node = d;
		var now = Date.now();
		clickElapsed = now-clickTime;
		clickTime = now;

		dblClickPrimed = (lastClickNode == mousedown_node);
		lastClickNode = mousedown_node;

		var i;

		if (d.selected && d3.event.ctrlKey) {
			d.selected = false;
			for (i=0;i<moving_set.length;i+=1) {
				if (moving_set[i].n === d) {
					moving_set.splice(i,1);
					break;
				}
			}
		} else {
			if (d3.event.shiftKey) {
				clearSelection();
				var cnodes = RED.nodes.getAllFlowNodes(mousedown_node);
				for (var n=0;n<cnodes.length;n++) {
					cnodes[n].selected = true;
					cnodes[n].dirty = true;
					moving_set.push({n:cnodes[n]});
				}
			} else if (!d.selected) {
				if (!d3.event.ctrlKey) {
					clearSelection();
				}
				mousedown_node.selected = true;
				console.warn("node mouse down happend" + this);
				moving_set.push({n:mousedown_node, rect:this});
			}
			selected_link = null;
			if (d3.event.button != 2) {
				mouse_mode = RED.state.MOVING;
				var mouse = d3.touches(this)[0]||d3.mouse(this);
				mouse[0] += d.x-d.w/2;
				mouse[1] += d.y-d.h/2;
				for (i=0;i<moving_set.length;i++) {
					moving_set[i].ox = moving_set[i].n.x;
					moving_set[i].oy = moving_set[i].n.y;
					moving_set[i].dx = moving_set[i].n.x-mouse[0];
					moving_set[i].dy = moving_set[i].n.y-mouse[1];
				}
				mouse_offset = d3.mouse(document.body);
				if (isNaN(mouse_offset[0])) {
					mouse_offset = d3.touches(document.body)[0];
				}
			}
		}
		d.dirty = true;
		updateSelection();
		redraw();
		d3.event.stopPropagation();
	}

	function nodeButtonClicked(d) {
		console.warn("node button pressed@"+d.name); // jannik add so that we can see, just maybe for future use of button?
		if (d._def.button.toggle) {
			d[d._def.button.toggle] = !d[d._def.button.toggle];
			d.dirty = true;
		}
		if (d._def.button.onclick) {
			d._def.button.onclick.call(d);
		}
		if (d.dirty) {
			redraw();
		}
		d3.event.preventDefault();
	}

	function showTouchMenu(obj,pos) {
		var mdn = mousedown_node;
		var options = [];
		options.push({name:"delete",disabled:(moving_set.length===0),onselect:function() {deleteSelection();}});
		options.push({name:"cut",disabled:(moving_set.length===0),onselect:function() {copySelection();deleteSelection();}});
		options.push({name:"copy",disabled:(moving_set.length===0),onselect:function() {copySelection();}});
		options.push({name:"paste",disabled:(clipboard.length===0),onselect:function() {importNodes(clipboard,true);}});
		options.push({name:"edit",disabled:(moving_set.length != 1),onselect:function() { RED.editor.edit(mdn);}});
		options.push({name:"select",onselect:function() {selectAll();}});
		options.push({name:"undo",disabled:(RED.history.depth() === 0),onselect:function() {RED.history.pop();}});

		RED.touch.radialMenu.show(obj,pos,options);
		resetMouseVars();
	}

	
	function checkRequirements(d) {
		//Add requirements
		d.requirementError = false;
		d.conflicts = new Array();
		d.requirements = new Array();
		
		RED.main.requirements.forEach(function(r) {
			if (r.type == d.type) d.requirements.push(r);
		});

		// above could be:
		// d.requirements = RED.main.requirements[d.type];
		// if the structure is changed a little

		//check for conflicts with other nodes:
		d.requirements.forEach(function(r) {
			RED.nodes.eachNode(function (n2) {
				if (n2 != d && n2.requirements != null ) {
					n2.requirements.forEach(function(r2) {
						if (r["resource"] == r2["resource"]) {
							if (r["shareable"] == false ) {
								var msg = "Conflict: shareable '"+r["resource"]+"'  "+d.name+" and "+n2.name;
								console.log(msg);
								msg = n2.name + " uses " + r["resource"] + ", too";
								d.conflicts.push(msg);
								d.requirementError = true;
							}
							//else
							if (r["setting"] != r2["setting"]) {
								var msg = "Conflict: "+ d.name + " setting['"+r["setting"]+"'] and "+n2.name+" setting['"+r2["setting"]+"']";
								console.log(msg);
								msg = n2.name + " has different settings: " + r["setting"] + " ./. " + r2["setting"];
								d.conflicts.push(msg);
								d.requirementError = true;
							}
						}
					});
				}
			});
		});		
	}
	
	function redraw_calcNewNodeSize(d)
	{
		//var l = d._def.label;
		//l = (typeof l === "function" ? l.call(d) : l)||"";
		var l = d.name ? d.name : d.id;
		
		if (d.inputs) // Jannik
		{
			d.w = Math.max(node_width,calculateTextWidth(l)+(d.inputs>0?7:0) );
			d.h = Math.max(node_height, (Math.max(d.outputs,d.inputs)||0) * 13 + 8);
		}
		else
		{
			d.w = Math.max(node_width,calculateTextWidth(l)+(d._def.inputs>0?7:0) );
			d.h = Math.max(node_height,(Math.max(d.outputs,d._def.inputs)||0) * 13 + 8);
		}
	}
	function redraw_nodeBadge(nodeRect, d)
	{
		var badge = nodeRect.append("svg:g").attr("class","node_badge_group");
		var badgeRect = badge.append("rect").attr("class","node_badge").attr("rx",5).attr("ry",5).attr("width",40).attr("height",15);
		badge.append("svg:text").attr("class","node_badge_label").attr("x",35).attr("y",11).attr('text-anchor','end').text(d._def.badge());
		if (d._def.onbadgeclick) {
			badgeRect.attr("cursor","pointer")
				.on("click",function(d) { d._def.onbadgeclick.call(d);d3.event.preventDefault();});
		}
	}
	
	function redraw_nodeButton(nodeRect, d)
	{
		var nodeButtonGroup = nodeRect.append('svg:g')
			.attr("transform",function(d) { return "translate("+((d._def.align == "right") ? 94 : -25)+",2)"; })
			.attr("class",function(d) { return "node_button "+((d._def.align == "right") ? "node_right_button" : "node_left_button"); });
		nodeButtonGroup.append('rect')
			.attr("rx",8)
			.attr("ry",8)
			.attr("width",32)
			.attr("height",node_height-4)
			.attr("fill","#eee");//function(d) { return d._def.color;})
		nodeButtonGroup.append('rect')
			.attr("x",function(d) { return d._def.align == "right"? 10:5})
			.attr("y",4)
			.attr("rx",5)
			.attr("ry",5)
			.attr("width",16)
			.attr("height",node_height-12)
			.attr("fill",function(d) { return d._def.color;})
			.attr("cursor","pointer")
			.on("mousedown",function(d) {if (!lasso) { d3.select(this).attr("fill-opacity",0.2);d3.event.preventDefault(); d3.event.stopPropagation();}})
			.on("mouseup",function(d) {if (!lasso) { d3.select(this).attr("fill-opacity",0.4);d3.event.preventDefault();d3.event.stopPropagation();}})
			.on("mouseover",function(d) {if (!lasso) { d3.select(this).attr("fill-opacity",0.4);}})
			.on("mouseout",function(d) {if (!lasso) {
				var op = 1;
				if (d._def.button.toggle) {
					op = d[d._def.button.toggle]?1:0.2;
				}
				d3.select(this).attr("fill-opacity",op);
			}})
			.on("click",nodeButtonClicked)
			.on("touchstart",nodeButtonClicked)
	}
	function redraw_nodeMainRect(nodeRect, d)
	{
		var mainRect = nodeRect.append("rect")
			.attr("class", "node")
			.classed("node_unknown",function(d) { return d.type == "unknown"; })
			.attr("rx", 6)
			.attr("ry", 6)
			.attr("fill",function(d) { return d._def.color;})
			.on("mouseup",nodeMouseUp)
			.on("mousedown",nodeMouseDown)
			.on("touchstart",function(d) {
				var obj = d3.select(this);
				var touch0 = d3.event.touches.item(0);
				var pos = [touch0.pageX,touch0.pageY];
				startTouchCenter = [touch0.pageX,touch0.pageY];
				startTouchDistance = 0;
				touchStartTime = setTimeout(function() {
					showTouchMenu(obj,pos);
				},touchLongPressTimeout);
				nodeMouseDown.call(this,d)
			})
			.on("touchend", function(d) {
				clearTimeout(touchStartTime);
				touchStartTime = null;
				if  (RED.touch.radialMenu.active()) {
					d3.event.stopPropagation();
					return;
				}
				nodeMouseUp.call(this,d);
			})
			.on("mouseover",function(d) {
				if (mouse_mode === 0) {
					var nodeRect = d3.select(this);
					nodeRect.classed("node_hovered",true);
					//console.log("node mouseover:" + d.name);
					var popoverText = "<b>" + d.type + "</b><br>";
					if (d.comment && (d.comment.trim().length != 0))
						popoverText+="<p>"+d.comment.replace("    ", "&emsp;").replace("\t", "&emsp;").replace(/\n/g, "<br>") + "</p>";
					//console.warn("popoverText:" +popoverText);
					$(current_popup_rect).popover("destroy"); // destroy prev
					if (d.type == "Function")
						showPopOver(this, true, popoverText, "bottom");
					else
						showPopOver(this, true, popoverText, "top");
				}
			})
			.on("mouseout",function(d) {
				var nodeRect = d3.select(this);
				nodeRect.classed("node_hovered",false);
				//console.log("node mouseout:" + d.name);
				$(current_popup_rect).popover("destroy");
			});
		//nodeRect.append("rect").attr("class", "node-gradient-top").attr("rx", 6).attr("ry", 6).attr("height",30).attr("stroke","none").attr("fill","url(#gradient-top)").style("pointer-events","none");
		//nodeRect.append("rect").attr("class", "node-gradient-bottom").attr("rx", 6).attr("ry", 6).attr("height",30).attr("stroke","none").attr("fill","url(#gradient-bottom)").style("pointer-events","none");
	}
	function redraw_nodeIcon(nodeRect, d)
	{
		nodeRect.selectAll(".node_icon_group").remove();
		
		var icon_group = nodeRect.append("g")
			.attr("class","node_icon_group")
			.attr("x",0).attr("y",0);

		var icon_shade = icon_group.append("rect")
			.attr("x",0).attr("y",0)
			.attr("class","node_icon_shade")
			.attr("width","30")
			.attr("stroke","none")
			.attr("fill","#000")
			.attr("fill-opacity","0.05")
			.attr("height",function(d){return Math.min(50,d.h-4);});

		var icon = icon_group.append("image")
			.attr("xlink:href","icons/"+d._def.icon)
			.attr("class","node_icon")
			.attr("x",0)
			.attr("width","30")
			.attr("height","30");

		var icon_shade_border = icon_group.append("path")
			.attr("d",function(d) { return "M 30 1 l 0 "+(d.h-2)})
			.attr("class","node_icon_shade_border")
			.attr("stroke-opacity","0.1")
			.attr("stroke","#000")
			.attr("stroke-width","2");

		if ("right" == d._def.align) {
			icon_group.attr('class','node_icon_group node_icon_group_'+d._def.align);
			icon_shade_border.attr("d",function(d) { return "M 0 1 l 0 "+(d.h-2)});
			//icon.attr('class','node_icon node_icon_'+d._def.align);
			//icon.attr('class','node_icon_shade node_icon_shade_'+d._def.align);
			//icon.attr('class','node_icon_shade_border node_icon_shade_border_'+d._def.align);
		}

		//if (d._def.inputs > 0 && d._def.align == null) {
		//    icon_shade.attr("width",35);
		//    icon.attr("transform","translate(5,0)");
		//    icon_shade_border.attr("transform","translate(5,0)");
		//}
		//if (d._def.outputs > 0 && "right" == d._def.align) {
		//    icon_shade.attr("width",35); //icon.attr("x",5);
		//}

		var img = new Image();
		img.src = "icons/"+d._def.icon;
		img.onload = function() {
			icon.attr("width",Math.min(img.width,30));
			icon.attr("height",Math.min(img.height,30));
			icon.attr("x",15-Math.min(img.width,30)/2);
			//if ("right" == d._def.align) {
			//    icon.attr("x",function(d){return d.w-img.width-1-(d.outputs>0?5:0);});
			//    icon_shade.attr("x",function(d){return d.w-30});
			//    icon_shade_border.attr("d",function(d){return "M "+(d.w-30)+" 1 l 0 "+(d.h-2);});
			//}
		};

		//icon.style("pointer-events","none");
		icon_group.style("pointer-events","none");			
	}
	function redraw_nodeText(nodeRect, d)
	{
		var text = nodeRect.append('svg:text').attr('class','node_label').attr('x', 38).attr('dy', '.35em').attr('text-anchor','start');
		if (d._def.align) {
			text.attr('class','node_label node_label_'+d._def.align);
			text.attr('text-anchor','end');
		}
	}
	function redraw_nodeStatus(nodeRect)
	{
		var status = nodeRect.append("svg:g").attr("class","node_status_group").style("display","none");

		var statusRect = status.append("rect").attr("class","node_status")
			.attr("x",6).attr("y",1).attr("width",9).attr("height",9)
			.attr("rx",2).attr("ry",2).attr("stroke-width","3");

		var statusLabel = status.append("svg:text")
			.attr("class","node_status_label")
			.attr('x',20).attr('y',9)
			.style({
				'stroke-width': 0,
				'fill': '#888',
				'font-size':'9pt',
				'stroke':'#000',
				'text-anchor':'start'
			});
	}
	function redraw_nodeInputs(nodeRect, d)
	{
		var numInputs = 0;
		if (d.inputs) // Jannik
			numInputs = d.inputs;
		else
			numInputs = d._def.inputs;
		
		var inputPorts = nodeRect.selectAll(".port_input");

		for (var i = 0; i < inputPorts.length; i++)
		{
			//console.warn(inputPorts[i]); // debug
			$(inputPorts[i]).popover("destroy");
		}
		inputPorts.remove();
		
		var inputlist = [];
		for (var n=0; n < numInputs; n++) {
			var link = RED.nodes.links.filter(function(l){return (l.target == d && l.targetPort == n);}); // used to see if any link is connected to the input
			
			var y = (d.h/2)-((numInputs-1)/2)*13;
			y = (y+13*n)-5;

			var rect = nodeRect.append("rect");
			inputlist[n] = rect;
			rect.attr("class","port port_input").attr("rx",3).attr("ry",3)
				.attr("y",y).attr("x",-5).attr("width",10).attr("height",10).attr("index",n);

			if (link && link.length > 0) {
				// this input already has a link connected, so disallow new links
				rect.on("mousedown",null)
					.on("touchstart", null)
					.on("mouseup", null)
					.on("touchend", null)
					.on("mouseover", nodeInput_mouseover) // used by popOver
					.on("mouseout", nodePort_mouseout) // used by popOver
					.classed("port_hovered",false);
			} else {
				// allow new link on inputs without any link connected
				rect.on("mousedown", (function(nn){return function(d){portMouseDown(d,1,nn);}})(n))
					.on("touchstart", (function(nn){return function(d){portMouseDown(d,1,nn);}})(n))
					.on("mouseup", (function(nn){return function(d){portMouseUp(d,1,nn);}})(n))
					.on("touchend", (function(nn){return function(d){portMouseUp(d,1,nn);}})(n))
					.on("mouseover", nodeInput_mouseover)
					.on("mouseout", nodePort_mouseout)
					//.on("mouseover", function(d) { var port = d3.select(this); port.classed("port_hovered",(mouse_mode!=RED.state.JOINING || mousedown_port_type != 1 ));})
					//.on("mouseout", function(d) { var port = d3.select(this); port.classed("port_hovered",false);})
					
			}
		}
		d.inputlist = inputlist;
	}
		
	function redraw_nodeOutputs(nodeRect, d)
	{
		var numOutputs = d.outputs;

		var y = (d.h/2)-((numOutputs-1)/2)*13;

		//if (d.type == "Mod") // debug test only
		//{
		//	console.error("before:" + d.ports);
		//	console.error("d3.range(numOutputs):" + d3.range(numOutputs));
		//}

		//d.ports = d.ports || d3.range(numOutputs);
		d.ports = d3.range(numOutputs);

		//if (d.type == "Mod") // debug test only
		//	console.error("after:" + d.ports);
		
			d._ports = nodeRect.selectAll(".port_output").data(d.ports);
		d._ports.enter().append("rect")
		    .attr("class","port port_output").attr("rx",3).attr("ry",3).attr("width",10).attr("height",10)
			.attr("nodeType",d.type)
			.on("mousedown",(function(){var node = d; return function(d,i){portMouseDown(node,0,i);}})() )
			.on("touchstart",(function(){var node = d; return function(d,i){portMouseDown(node,0,i);}})() )
			.on("mouseup",(function(){var node = d; return function(d,i){portMouseUp(node,0,i);}})() )
			.on("touchend",(function(){var node = d; return function(d,i){portMouseUp(node,0,i);}})() )
			.on("mouseover", nodeOutput_mouseover)
			.on("mouseout", nodePort_mouseout)
			//.on("mouseover",function(d,i) { var port = d3.select(this); port.classed("port_hovered",(mouse_mode!=RED.state.JOINING || mousedown_port_type !== 0 ));})
			//.on("mouseout",function(d,i) { var port = d3.select(this); port.classed("port_hovered",false);});
		d._ports.exit().remove();
		if (d._ports) {
			numOutputs = d.outputs || 1;
			y = (d.h/2)-((numOutputs-1)/2)*13;
			var x = d.w - 5;
			d._ports.each(function(d,i) {
				var port = d3.select(this);
				port.attr("y",(y+13*i)-5).attr("x",x);
			});
		}
	}
	function redraw_links()
	{
		var link = vis.selectAll(".link").data(RED.nodes.links.filter(function(d) { return d.source.z == activeWorkspace && d.target.z == activeWorkspace }),function(d) { return d.source.id+":"+d.sourcePort+":"+d.target.id+":"+d.targetPort;});

		var linkEnter = link.enter().insert("g",".node").attr("class","link");

		linkEnter.each(function(d,i) {
			var l = d3.select(this);
			l.append("svg:path").attr("class","link_background link_path")
			   .on("mousedown",function(d) {
					mousedown_link = d;
					clearSelection();
					selected_link = mousedown_link;
					updateSelection();
					redraw();
					d3.event.stopPropagation();
				})
				.on("touchstart",function(d) {
					mousedown_link = d;
					clearSelection();
					selected_link = mousedown_link;
					updateSelection();
					redraw();
					d3.event.stopPropagation();
				});
			l.append("svg:path").attr("class","link_outline link_path");
			l.append("svg:path").attr("class","link_line link_path");
		});

		link.exit().remove();

		var links = vis.selectAll(".link_path");
		links.attr("d",function(d){
				var numOutputs = d.source.outputs || 1;
				var sourcePort = d.sourcePort || 0;
				var ysource = -((numOutputs-1)/2)*13 +13*sourcePort;
				
				var numInputs = 0;
				if (d.target.inputs) numInputs = d.target.inputs || 1; //Jannik
				else numInputs = d.target._def.inputs || 1;
				
				var targetPort = d.targetPort || 0;
				var ytarget = -((numInputs-1)/2)*13 +13*targetPort;

				var dy = (d.target.y+ytarget)-(d.source.y+ysource);
				var dx = (d.target.x-d.target.w/2)-(d.source.x+d.source.w/2);
				var delta = Math.sqrt(dy*dy+dx*dx);
				var scale = lineCurveScale;
				var scaleY = 0;
				if (delta < node_width) {
					scale = 0.75-0.75*((node_width-delta)/node_width);
				}

				if (dx < 0) {
					scale += 2*(Math.min(5*node_width,Math.abs(dx))/(5*node_width));
					if (Math.abs(dy) < 3*node_height) {
						scaleY = ((dy>0)?0.5:-0.5)*(((3*node_height)-Math.abs(dy))/(3*node_height))*(Math.min(node_width,Math.abs(dx))/(node_width)) ;
					}
				}

				d.x1 = d.source.x+d.source.w/2;
				d.y1 = d.source.y+ysource;
				d.x2 = d.target.x-d.target.w/2;
				d.y2 = d.target.y+ytarget;

				return "M "+(d.x1)+" "+(d.y1)+
					" C "+(d.x1+scale*node_width)+" "+(d.y1+scaleY*node_height)+" "+
					(d.x2-scale*node_width)+" "+(d.y2-scaleY*node_height)+" "+
					(d.x2)+" "+d.y2;
		});

		link.classed("link_selected", function(d) { return d === selected_link || d.selected; });
		link.classed("link_unknown",function(d) { return d.target.type == "unknown" || d.source.type == "unknown"});
	}
	function nodeOutput_mouseover(pi) // here d is the portindex
	{
		var port = d3.select(this); 
		
		$(this).popover("destroy"); // destroy prev
		var data = getIOpinInfo(this, undefined, pi);
		showPopOver(this, true, data, "left");
						
		port.classed("port_hovered",(mouse_mode!=RED.state.JOINING || mousedown_port_type != 0 ));
		//console.log("nodeOutput_mouseover: " + this.getAttribute("nodeZ") + "." + this.getAttribute("nodeName") +" , port:" + pi);
	}
	function nodeInput_mouseover(d) // here d is the node
	{
		var port = d3.select(this); 
		
		$(this).popover("destroy"); // destroy prev
		var data = getIOpinInfo(this, d, this.getAttribute("index")); // d is the node
		showPopOver(this, true, data, "right");
						
		port.classed("port_hovered",(mouse_mode!=RED.state.JOINING || mousedown_port_type != 1 ));
		//console.log("nodeInput_mouseover: " + d.name +" , port:" + this.getAttribute("index"));
	}
	function nodePort_mouseout(d)
	{
		var port = d3.select(this); 
		port.classed("port_hovered",false);
		$(this).popover("destroy"); // destroy prev
	}
	/*********************************************************************************************************************************/
	/*********************************************************************************************************************************/
	/*********************************************************************************************************************************/
	/*********************************************************************************************************************************/
	/*********************************************************************************************************************************/
	/*********************************************************************************************************************************/
	/*********************************************************************************************************************************/
	/*********************************************************************************************************************************/
	/*********************************************************************************************************************************/
	function redraw() {
		const t0 = performance.now();
		//console.trace("redraw");
		
		//console.log("redraw");
		
		vis.attr("transform","scale("+scaleFactor+")");
		outer.attr("width", space_width*scaleFactor).attr("height", space_height*scaleFactor);

		if (mouse_mode != RED.state.JOINING) {
			// Don't bother redrawing nodes if we're drawing links
			const t2 = performance.now();
			var node = vis.selectAll(".nodegroup").data(RED.nodes.nodes.filter(function(d) { return d.z == activeWorkspace }),function(d){return d.id});
			const t3 = performance.now();
			console.log('vis.selectAll: ' + (t3-t2) +' milliseconds.');

			var updatedClassTypes =	false; // flag so that it only run once at each redraw()

			var nodeExit = node.exit().remove();
			nodeExit.each(function(d,i) // this happens only when a node exits(is removed) from the current workspace.
			{
				//console.error("redraw nodeExit:" + d.type);
				if (d.type == "TabInput" || d.type == "TabOutput")
				{
					if (!updatedClassTypes) { updatedClassTypes = true; RED.nodes.updateClassTypes(); }
				}
			});

			var nodeEnter = node.enter().insert("svg:g").attr("class", "node nodegroup");
			nodeEnter.each(function(d,i) // this happens only when a node enter(is added) to the current workspace.
			{
				//console.error("redraw nodeEnter:" + d.type);
				if (d.type == "TabInput" || d.type == "TabOutput")
				{
					if (!updatedClassTypes) { updatedClassTypes = true; RED.nodes.updateClassTypes(); }
				}

				var nodeRect = d3.select(this);
				nodeRect.attr("id",d.id);
				redraw_calcNewNodeSize(d);
				
				if (d._def.category.startsWith("output") || d._def.category.startsWith("input")) // only need to check I/O
				{	
					checkRequirements(d); // this update nodes that allready exist
					if (d.requirementError) console.warn("@nodeEnter reqError on:" + d.name);
					redraw_nodeReqError(nodeRect, d);
					redraw_paletteNodesReqError(d);
				}

				if (d._def.badge) redraw_nodeBadge(nodeRect, d);
				if (d._def.button) redraw_nodeButton(nodeRect, d);
				redraw_nodeMainRect(nodeRect, d);
				if (d._def.icon) redraw_nodeIcon(nodeRect, d);
				redraw_nodeInputs(nodeRect, d);
				redraw_nodeOutputs(nodeRect, d);
				redraw_nodeText(nodeRect, d);
				//redraw_nodeStatus(nodeRect);

				//nodeRect.append("circle").attr({"class":"centerDot","cx":0,"cy":0,"r":5});
				// never show these little status icons
				// people try clicking on them, thinking they're buttons
				// or some sort of user interface widget
				//nodeRect.append("path").attr("class","node_error").attr("d","M 3,-3 l 10,0 l -5,-8 z");
				//nodeRect.append("image").attr("class","node_error hidden").attr("xlink:href","icons/node-error.png").attr("x",0).attr("y",-6).attr("width",10).attr("height",9);
				//nodeRect.append("image").attr("class","node_changed hidden").attr("xlink:href","icons/node-changed.png").attr("x",12).attr("y",-6).attr("width",10).attr("height",10);
				nodeRect.append("image").attr("class","node_reqerror hidden").attr("xlink:href","icons/error.png").attr("x",0).attr("y",-12).attr("width",20).attr("height",20);
			});

			node.each(function(d,i) { // redraw all nodes in active workspace
					var nodeRect = d3.select(this);

					if (d._def.category.startsWith("output") || d._def.category.startsWith("input")) // only need to check I/O
					{	
						checkRequirements(d); // this update nodes that allready exist
						if (d.requirementError) console.warn("@node.each reqError on:" + d.name);
						redraw_nodeReqError(nodeRect, d);
					}
					if (d.dirty) {
						//if (d.x < -50) deleteSelection();  // Delete nodes if dragged back to palette
						if (d.resize) {
							
							redraw_calcNewNodeSize(d);
							redraw_nodeInputs(nodeRect, d);
							redraw_nodeOutputs(nodeRect, d);
							d.resize = false;
						}
						//console.log("redraw stuff");
						
						redraw_paletteNodesReqError(d);
						redraw_other(nodeRect, d);
						d.dirty = false;
						
					}
			});

		}
		redraw_links();

		if (d3.event) {
			d3.event.preventDefault();
		}
		const t1 = performance.now();
		console.log('redraw took: ' + (t1-t0) +' milliseconds.');
	}
	function redraw_paletteNodesReqError(d)
	{
		var e1 = document.getElementById("palette_node_"+d.type);
		var e2 = e1.getElementsByClassName("palette_req_error")[0]; // palette_req_error is using a style, where the position of the icon is defined
		e2.addEventListener("click", 
							function(){RED.notify('Conflicts:<ul><li>'+d.conflicts.join('</li><li>')+'</li></ul>',null,false, 5000);},
							{once: true});
		if (d.requirementError)
			e2.classList.remove("hidden");
		else
			e2.classList.add("hidden");
	}
	function redraw_nodeReqError(nodeRect, d)
	{
		nodeRect.selectAll(".node_reqerror")
			.attr("x",function(d){return d.w-25-(d.changed?13:0)})
			.classed("hidden",function(d){ return !d.requirementError; })
			.on("click", function(){RED.notify(
				'Conflicts:<ul><li>'+d.conflicts.join('</li><li>')+'</li></ul>',
				null,
				false,
				5000
				)});
	}
	function redraw_other(nodeRect, d) // this contains the rest until they get own functions
	{
		//nodeRect.selectAll(".centerDot").attr({"cx":function(d) { return d.w/2;},"cy":function(d){return d.h/2}});
		nodeRect.attr("transform", function(d) { return "translate(" + (d.x-d.w/2) + "," + (d.y-d.h/2) + ")"; });
		nodeRect.selectAll(".node")
			.attr("width",function(d){return d.w})
			.attr("height",function(d){return d.h})
			.classed("node_selected",function(d) { return d.selected; })
			.classed("node_highlighted",function(d) { return d.highlighted; })
		;
		//nodeRect.selectAll(".node-gradient-top").attr("width",function(d){return d.w});
		//nodeRect.selectAll(".node-gradient-bottom").attr("width",function(d){return d.w}).attr("y",function(d){return d.h-30});

		nodeRect.selectAll(".node_icon_group_right").attr('transform', function(d){return "translate("+(d.w-30)+",0)"});
		nodeRect.selectAll(".node_label_right").attr('x', function(d){return d.w-38});
		//nodeRect.selectAll(".node_icon_right").attr("x",function(d){return d.w-d3.select(this).attr("width")-1-(d.outputs>0?5:0);});
		//nodeRect.selectAll(".node_icon_shade_right").attr("x",function(d){return d.w-30;});
		//nodeRect.selectAll(".node_icon_shade_border_right").attr("d",function(d){return "M "+(d.w-30)+" 1 l 0 "+(d.h-2)});
		
		nodeRect.selectAll(".node_icon").attr("y",function(d){return (d.h-d3.select(this).attr("height"))/2;});
		nodeRect.selectAll(".node_icon_shade").attr("height",function(d){return d.h;});
		nodeRect.selectAll(".node_icon_shade_border").attr("d",function(d){ return "M "+(("right" == d._def.align) ?0:30)+" 1 l 0 "+(d.h-2)});


		nodeRect.selectAll('text.node_label').text(function(d,i){
				/* if (d._def.label) {
					if (typeof d._def.label == "function") {
						return d._def.label.call(d);
					} else {
						return d._def.label;
					}
				}
				return "n.a.";
				 */
				return d.name ? d.name : d.id;
		})
			.attr('y', function(d){return (d.h/2)-1;})
			.attr('class',function(d){
				return 'node_label'+
				(d._def.align?' node_label_'+d._def.align:'')+
				(d._def.label?' '+(typeof d._def.labelStyle == "function" ? d._def.labelStyle.call(d):d._def.labelStyle):'') ;
		});
		//nodeRect.selectAll(".node_tools").attr("x",function(d){return d.w-35;}).attr("y",function(d){return d.h-20;});

		/*nodeRect.selectAll(".node_changed")
			.attr("x",function(d){return d.w-10})
			.classed("hidden",function(d) { return !d.changed; });*/ // this is disabled above

		/*nodeRect.selectAll(".node_error")
			.attr("x",function(d){return d.w-10-(d.changed?13:0)})
			.classed("hidden",function(d) { return d.valid; });*/  // this is disabled above

		nodeRect.selectAll(".port_input").each(function(d,i) {
			var port = d3.select(this);
			var numInputs = 0;
			if (d.inputs) numInputs = d.inputs;
			else numInputs = d._def.inputs;
				
			var y = (d.h/2)-((numInputs-1)/2)*13;
			y = (y+13*i)-5;
			port.attr("y",y)
		});

		

		
		/*nodeRect.selectAll('.node_right_button').attr("transform",function(d){
				var x = d.w-6;
				if (d._def.button.toggle && !d[d._def.button.toggle]) {
					x = x - 8;
				}
				return "translate("+x+",2)";
		});
		nodeRect.selectAll('.node_right_button rect').attr("fill-opacity",function(d){
				if (d._def.button.toggle) {
					return d[d._def.button.toggle]?1:0.2;
				}
				return 1;
		});*/ // maybe we don't need this

		//nodeRect.selectAll('.node_right_button').attr("transform",function(d){return "translate("+(d.w - d._def.button.width.call(d))+","+0+")";}).attr("fill",function(d) {
		//         return typeof d._def.button.color  === "function" ? d._def.button.color.call(d):(d._def.button.color != null ? d._def.button.color : d._def.color)
		//});
		/*
		nodeRect.selectAll('.node_badge_group').attr("transform",function(d){return "translate("+(d.w-40)+","+(d.h+3)+")";});
		nodeRect.selectAll('text.node_badge_label').text(function(d,i) {
			 if (d._def.badge) {
				if (typeof d._def.badge == "function") {
					return d._def.badge.call(d);
				} else {
					return d._def.badge;
				}
			}
			//return "";
			return d.name ? d.name : d.id;
		});*/
		/*if (!showStatus || !d.status) {
			nodeRect.selectAll('.node_status_group').style("display","none");
		} else {
			nodeRect.selectAll('.node_status_group').style("display","inline").attr("transform","translate(3,"+(d.h+3)+")");
			var fill = status_colours[d.status.fill]; // Only allow our colours for now
			if (d.status.shape == null && fill == null) {
				nodeRect.selectAll('.node_status').style("display","none");
			} else {
				var style;
				if (d.status.shape == null || d.status.shape == "dot") {
					style = {
						display: "inline",
						fill: fill,
						stroke: fill
					};
				} else if (d.status.shape == "ring" ){
					style = {
						display: "inline",
						fill: '#fff',
						stroke: fill
					}
				}
				nodeRect.selectAll('.node_status').style(style);
			}
			if (d.status.text) {
				nodeRect.selectAll('.node_status_label').text(d.status.text);
			} else {
				nodeRect.selectAll('.node_status_label').text("");
			}
		}*/
	}

	function doSort (arr) {
		arr.sort(function (a, b) {
			var nameA = a.name ? a.name : a.id;
			var nameB = b.name ? b.name : b.id;
			return nameA.localeCompare(nameB, 'en', {numeric: 'true'});
		});
	}

	function setNewCoords (lastX, lastY, arr) {
		var x = lastX;
		var y = lastY;
		for (var i = 0; i < arr.length; i++) {
			var node = arr[i];
			var name = node.name ? node.name : node.id;
			var def = node._def;
			var dH = Math.max(RED.view.defaults.height, (Math.max(def.outputs, def.inputs) || 0) * 15);
			x = lastX + Math.max(RED.view.defaults.width, RED.view.calculateTextWidth(name) + (def.inputs > 0 ? 7 : 0));
			node.x = x;
			node.y = y + dH/2;
			y = y + dH + 15;
			node.dirty = true;
		}
		return { x: x, y: y };
	}

	function arrangeAll() {
		var ioNoIn = [];
		var ioInOut = [];
		var ioMultiple = [];
		var ioNoOut = [];
		var ioCtrl = [];

		RED.nodes.eachNode(function (node) {
			if (node.z != activeWorkspace) return;
			var inputs = 0;
			if (node.inputs == undefined)
				inputs = node._def.inputs;
			else
				inputs = node.inputs;

			if (inputs == 0 && node._def.outputs == 0) {
				ioCtrl.push(node);
			} else if (inputs == 0) {
				ioNoIn.push(node);
			} else if (node._def.outputs == 0) {
				ioNoOut.push(node);
			} else if (inputs == 1 && node._def.outputs == 1) {
				ioInOut.push(node);
			} else if (inputs > 1 || node._def.outputs > 1) {
				ioMultiple.push(node);
			}
		});

		var cols = new Array(ioNoIn, ioInOut, ioMultiple, ioNoOut, ioCtrl);
		var lowestY = 0;

		for (var i = 0; i < cols.length; i++) {
			var dX = ((i < cols.length - 1) ?  i : 0) * (RED.view.defaults.width * 2) + (RED.view.defaults.width / 2) + 15;
			var dY = ((i < cols.length - 1) ?  (RED.view.defaults.height / 4) : lowestY) + 15;
			var startX = 0;
			var startY = 0;

			doSort(cols[i]);
			var last = setNewCoords(startX + dX, startY + dY, cols[i]);
			lowestY = Math.max(lowestY, last.y);
			startX = ((i < cols.length - 1) ? last.x : 0) + (RED.view.defaults.width) * 4;
			startY = lowestY + (RED.view.defaults.height * 1.5);
		}
		RED.storage.update();
		redraw();
	}

	RED.keyboard.add(/* z */ 90,{ctrl:true},function(){RED.history.pop();});
	RED.keyboard.add(/* o */ 79,{ctrl:true},function(){arrangeAll();d3.event.preventDefault();});
	RED.keyboard.add(/* a */ 65,{ctrl:true},function(){selectAll();d3.event.preventDefault();});
	RED.keyboard.add(/* = */ 187,{ctrl:true},function(){zoomIn();d3.event.preventDefault();});
	RED.keyboard.add(/* - */ 189,{ctrl:true},function(){zoomOut();d3.event.preventDefault();});
	RED.keyboard.add(/* 0 */ 48,{ctrl:true},function(){zoomZero();d3.event.preventDefault();});
	RED.keyboard.add(/* v */ 86,{ctrl:true},function(){importNodes(clipboard);d3.event.preventDefault();});
	RED.keyboard.add(/* e */ 69,{ctrl:true},function(){showExportNodesDialog();d3.event.preventDefault();});
	RED.keyboard.add(/* i */ 73,{ctrl:true},function(){showImportNodesDialog(true);d3.event.preventDefault();});

	// TODO: 'dirty' should be a property of RED.nodes - with an event callback for ui hooks
	function setDirty(d) {
		dirty = d;
		if (dirty) {
			$("#btn-deploy").removeClass("disabled").addClass("btn-danger");
			RED.storage.update();
		} else {
			$("#btn-deploy").addClass("disabled").removeClass("btn-danger");
		}
	}

	/**
	 * Imports a new collection of nodes from a JSON String.
	 *  - all get new IDs assigned
	 *  - all 'selected'
	 *  - attached to mouse for placing - 'IMPORT_DRAGGING'
	 */
	function importNodes(newNodesStr,touchImport) {

		var createNewIds = true;
		var useStorage = false;

		if ($("#node-input-arduino").prop('checked') === true) {
			var nodesJSON = RED.arduino.import.cppToJSON(newNodesStr);
			if (nodesJSON.count <= 0) {
				var note = "No nodes imported!";
				RED.notify("<strong>Note</strong>: " + note, "warning");
			}

			newNodesStr = nodesJSON.data;
			createNewIds = false;

			if (useStorage) {
				RED.storage.clear();
				localStorage.setItem("audio_library_guitool", newNodesStr);
				RED.storage.load();
				redraw();
				return;
			}
		}

		try {
			var result = RED.nodes.import(newNodesStr,createNewIds);
			if (result) {
				var new_nodes = result[0];
				var new_links = result[1];
				var new_ms = new_nodes.map(function(n) { n.z = activeWorkspace; return {n:n};});
				var new_node_ids = new_nodes.map(function(n){ return n.id; });

				// TODO: pick a more sensible root node
				var root_node = new_ms[0].n;
				var dx = root_node.x;
				var dy = root_node.y;

				if (mouse_position == null) {
					mouse_position = [0,0];
				}

				var minX = 0;
				var minY = 0;
				var i;
				var node;

				for (i=0;i<new_ms.length;i++) {
					node = new_ms[i];
					node.n.selected = true;
					node.n.changed = true;
					node.n.x -= dx - mouse_position[0];
					node.n.y -= dy - mouse_position[1];
					node.dx = node.n.x - mouse_position[0];
					node.dy = node.n.y - mouse_position[1];
					minX = Math.min(node.n.x-node_width/2-5,minX);
					minY = Math.min(node.n.y-node_height/2-5,minY);
				}
				for (i=0;i<new_ms.length;i++) {
					node = new_ms[i];
					node.n.x -= minX;
					node.n.y -= minY;
					node.dx -= minX;
					node.dy -= minY;
				}
				if (!touchImport) {
					mouse_mode = RED.state.IMPORT_DRAGGING;
				}

				RED.keyboard.add(/* ESCAPE */ 27,function(){
						RED.keyboard.remove(/* ESCAPE */ 27);
						clearSelection();
						RED.history.pop();
						mouse_mode = 0;
				});

				RED.history.push({t:'add',nodes:new_node_ids,links:new_links,dirty:RED.view.dirty()});

				clearSelection();
				moving_set = new_ms;

				redraw();
			}
		} catch(error) {
			console.log(error);
			RED.notify("<strong>Error</strong>: "+error,"error");
		}
	}

	function getForm(formId, key, callback) {
		// server test switched off - test purposes only
		var patt = new RegExp(/^[http|https]/);
		var server = false && patt.test(location.protocol);
		var form = $("<h2>No form found.</h2>");

		if (!server) {
			data = $("script[data-template-name|='" + key + "']").html();
			//console.log('%c' + typeof data + "%c"+ data, 'background: #bada55; color: #555 ', 'background: #555; color: #bada55 ');
			form = $("#" + formId);
			$(form).empty();
			$(form).append(data);
			if(typeof callback == 'function') {
				callback.call(this, form);
			}
		} else {
			var frmPlugin = "resources/form/" + key + ".html";
			$.get(frmPlugin, function(data) {
				form = $("#" + formId);
				$(form).empty();
				$(form).append(data);
				if(typeof callback == 'function') {
					callback.call(this, form);
				}
			});
		}

		return form;
	}

	$('#btn-import-json').click(function() {showImportNodesDialog(false);});
	$('#btn-import-arduino').click(function() {showImportNodesDialog(true);});
	$('#btn-export-clipboard').click(function() {showExportNodesDialog();});
	$('#btn-export-library').click(function() {showExportNodesLibraryDialog();});

	function showExportNodesDialog() {
		mouse_mode = RED.state.EXPORT;
		var nns = RED.nodes.createExportableNodeSet(moving_set);
		//$("#dialog-form").html(getForm("dialog-form", "export-clipboard-dialog"));
		var frm = getForm("dialog-form", "export-clipboard-dialog", function (d, f) {
			$("#node-input-export").val(JSON.stringify(nns)).focus(function() {
				var textarea = $(this);
				textarea.select();
				textarea.mouseup(function() {
						textarea.unbind("mouseup");
						return false;
				});
			}).focus();
		$( "#dialog" ).dialog("option","title","Export nodes to clipboard").dialog( "open" );
		});
	}

	function showExportNodesLibraryDialog() {
		mouse_mode = RED.state.EXPORT;
		var nns = RED.nodes.createExportableNodeSet(moving_set);
		//$("#dialog-form").html(this.getForm('export-library-dialog'));
		getForm("dialog-form", "export-library-dialog", function(d, f) {
		$("#node-input-filename").attr('nodes',JSON.stringify(nns));
		$( "#dialog" ).dialog("option","title","Export nodes to library").dialog( "open" );
		});
	}

	function showImportNodesDialog(arduino_code) {
		mouse_mode = RED.state.IMPORT;
		//$("#dialog-form").html(this.getForm('import-dialog'));
		getForm("dialog-form", "import-dialog", function(d, f) {
		$("#node-input-import").val("");
		$( "#node-input-arduino" ).prop('checked', arduino_code);
		$( "#dialog" ).dialog("option","title","Import nodes").dialog( "open" );
		});
	}

	function showRenameWorkspaceDialog(id) {
		var ws = RED.nodes.workspace(id);
		$( "#node-dialog-rename-workspace" ).dialog("option","workspace",ws);

		if (workspace_tabs.count() == 1) {
			$( "#node-dialog-rename-workspace").next().find(".leftButton")
				.prop('disabled',true)
				.addClass("ui-state-disabled");
		} else {
			$( "#node-dialog-rename-workspace").next().find(".leftButton")
				.prop('disabled',false)
				.removeClass("ui-state-disabled");
		}
		$( "#node-input-export-workspace" ).prop('checked',  ws.export);
		$( "#node-input-workspace-name" ).val(ws.label);
		$( "#node-dialog-rename-workspace" ).dialog("open");
	}

	$("#node-dialog-rename-workspace form" ).submit(function(e) { e.preventDefault();});
	$( "#node-dialog-rename-workspace" ).dialog({
		modal: true,
		autoOpen: false,
		width: 500,
		title: "Rename sheet",
		buttons: [
			{
				class: 'leftButton',
				text: "Delete",
				click: function() {
					var workspace = $(this).dialog('option','workspace');
					$( this ).dialog( "close" );
					deleteWorkspace(workspace.id);
				}
			},
			{
				text: "Ok",
				click: function() {
					var workspace = $(this).dialog('option','workspace');
					var label = $( "#node-input-workspace-name" ).val();
					if (workspace.label != label) {

						if (RED.nodes.workspaceNameCheck(label)) // Jannik add start
						{
							RED.notify("<strong>Warning</strong>: Name:"+label + " allready exist, choose annother name.","warning");
							return; // abort name change if name allready exist
						} 
						RED.nodes.workspaceNameChanged(workspace.label, label); // Jannik add end

						workspace.label = label;

						// update the tab text
						var link = $("#workspace-tabs a[href='#"+workspace.id+"']");
						link.attr("title",label);
						link.text(label);
						// update the menu item text
						var menuItem = $("#workspace-menu-list a[href='#"+workspace.id+"']");
						menuItem.attr("title",label);
						menuItem.text(label);

						RED.view.dirty(true);

					}
					var exportNew = $( "#node-input-export-workspace" ).prop('checked')
					if (workspace.export != exportNew)
					{
						workspace.export = exportNew;
						var link = $("#workspace-tabs a[href='#"+workspace.id+"']");
						if (!exportNew)
							link.attr("style", "color:#b3b3b3;");
						else // 
							link.attr("style", "color:#000000;");
					}
					console.warn("exportWorkspace:"+workspace.export);

					$( this ).dialog( "close" );
				}
			},
			{
				text: "Cancel",
				click: function() {
					$( this ).dialog( "close" );
				}
			}
		],
		open: function(e) {
			RED.keyboard.disable();
		},
		close: function(e) {
			RED.keyboard.enable();
		}
	});
	$( "#node-dialog-delete-workspace" ).dialog({
		modal: true,
		autoOpen: false,
		width: 500,
		title: "Confirm delete",
		buttons: [
			{
				text: "Ok",
				click: function() {
					var workspace = $(this).dialog('option','workspace');
					RED.view.removeWorkspace(workspace);
					var historyEvent = RED.nodes.removeWorkspace(workspace.id);
					historyEvent.t = 'delete';
					historyEvent.dirty = dirty;
					historyEvent.workspaces = [workspace];
					RED.history.push(historyEvent);
					RED.view.dirty(true);
					$( this ).dialog( "close" );
				}
			},
			{
				text: "Cancel",
				click: function() {
					$( this ).dialog( "close" );
				}
			}
		],
		open: function(e) {
			RED.keyboard.disable();
		},
		close: function(e) {
			RED.keyboard.enable();
		}

	});
	
	function getIOpinInfo(pinRect, node, index)
	{
		var classAttr = pinRect.getAttribute("class");
		//console.log("classAttr:"+classAttr); // development debug
		var portType;
		var nodeType;
		if (classAttr == "port port_input")
		{
			nodeType = node.type;
			portType = "In";
		}
		else if (classAttr == "port port_output")
		{
			nodeType = pinRect.getAttribute("nodeType");
			portType = "Out";
			
		}
		
		var data = $("script[data-help-name|='" + nodeType + "']");
		var data2 = $("<div/>").append(data.html()).children("table").first().children("tbody").html();
		
		var portName = portType + " " + index;

		if (!data2 || (data2 == null)) // shows workspace user custom class io
		{
			// TODO: extract portinfo from class
			if (RED.nodes.isClass(nodeType))
			{
				var wsId = RED.nodes.getWorkspaceIdFromClassName(nodeType);
				portName = portName + ": " + RED.nodes.getClassIOportName(wsId, "Tab"+portType+ "put", index);
			}
			data2 = $("<div/>").append("<p>" + portName + "</p></div>").html();
		}
		/*else if (nodeType == "AudioMixer" && portType == "In")
		{
			data2 = $("<div/>").append("<p>" + portName + ": Input Signal #" + (Number(index) + 1) + "</p></div>").html();
		}*/
		else // here we must extract info from Audio Connections table
		{
			var tableRows = data2.split("\n");
			for (var i = 1; i < tableRows.length; i++)
			{
				var tableCols = tableRows[i].split("</td><td>");
				if (tableCols.length < 2) continue;

				var pin = tableCols[0];
				var desc = tableCols[1];
				pin = pin.substring(pin.lastIndexOf(">") + 1);
				desc = desc.substring(0, desc.indexOf("<"));

				if (pin == portName)
				{
					data2 = $("<div/>").append("<p>" + pin + ": " + desc + "</p></div>").html();
					//console.log(pin + " " + desc); // development debug
					break;
				}
			}
			console.log("table contens: type("+ typeof data2 + "):\n"+ data2); // development debug
		}
		//console.log(data2); // development debug
		return data2;
	}

	function showPopOver(rect, htmlMode, content,placement)
	{
		if (placement == null) placement = "top";
		current_popup_rect = rect;
		var options = {
			placement: placement,
			trigger: "manual",
			html: htmlMode,
			container:'body',
			rootClose:true, // failsafe
			content : content
		};
		$(rect).popover(options).popover("show");
		//console.warn("content type:" + typeof content);
		//console.warn("content:" + content);
		//console.warn("showPopOver retVal:" + Object.getOwnPropertyNames(retVal)); // debug
		//console.warn("showPopOver retVal.context:" + retVal.context); // debug
		//console.warn("showPopOver retVal.length:" + retVal.length); // debug
	}

	function setShowWorkspaceToolbarVisible(state)
	{
		RED.nodes.showWorkspaceToolbar = state;
		if (state)
		{
			$("#workspace-toolbar").show();
			$("#chart").css("top", 67);
		}
		else
		{
			$("#workspace-toolbar").hide();
			$("#chart").css("top", 31);
		}
	}

	return {
		showHideGrid:showHideGrid,
		showHideGridH: function (state) { showGridH = state; showHideGridH(state); },
		showHideGridVmajor: function (state) { showGridVmajor = state; showHideGridVmajor(state); },
		showHideGridVminor: function (state) { showGridVminor = state; showHideGridVminor(state); },
		showGridH:showGridH,
		showGridVmajor:showGridVmajor,
		showGridVminor:showGridVminor,
		setSnapToGrid:setSnapToGrid,
		snapToGrid: snapToGrid,
		setSnapToGridXsize: function (size) { snapToGridXsize = size;},
		setSnapToGridYsize: function (size) { snapToGridYsize = size;},
		snapToGridXsize:snapToGridXsize,
		snapToGridYsize:snapToGridYsize,
		setShowWorkspaceToolbarVisible:setShowWorkspaceToolbarVisible,
		showWorkspaceToolbar:showWorkspaceToolbar,
		lineCurveScale:lineCurveScale,
		setLineCurveScale: function (value) { lineCurveScale = value; redraw_links()},
		state:function(state) {
			if (state == null) {
				return mouse_mode
			} else {
				mouse_mode = state;
			}
		},
		addWorkspace: function(ws) {
			workspace_tabs.addTab(ws); // see tabs.js
			//workspace_tabs.resize(); // see tabs.js // this is not needed because workspace_tabs.addTab() does it internally
		},
		removeWorkspace: function(ws) {
			workspace_tabs.removeTab(ws.id); // see tabs.js
		},
		getWorkspace: function() {
			return activeWorkspace;
		},
		showWorkspace: function(id) {
			workspace_tabs.activateTab(id); // see tabs.js
		},
		redraw: redraw,
		dirty: function(d) {
			if (d == null) {
				return dirty;
			} else {
				setDirty(d);
			}
		},
		importNodes: importNodes,
		resize: function() {
			workspace_tabs.resize();
		},
		status: function(s) {
			showStatus = s;
			RED.nodes.eachNode(function(n) { n.dirty = true;});
			//TODO: subscribe/unsubscribe here
			redraw();
		},
		getForm: getForm,
		calculateTextWidth: calculateTextWidth,
		showExportNodesDialog: showExportNodesDialog,
		defaults: {
			width: node_width,
			height: node_height
		}
	};
})();
