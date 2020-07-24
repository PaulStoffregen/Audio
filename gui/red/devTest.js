/** Added in addition to original Node-Red source, for audio system visualization
 * this file is intended for testing new functionality that is in development
 * when the functions are completed they should be removed from this file 
 * and put at the right file.
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

RED.devTest = (function() {
    
    function console_logColor(data)
    {
        console.log('%c' + typeof data + " %c"+ data, 'background: #bada55; color: #555 ', 'background: #555; color: #bada55 ');
    }
    

    function createAndPrintNewWsStruct()
	{
		console.warn("new ws struct:");
        
        for (var wsi = 0; wsi < RED.nodes.workspaces.length; wsi++)
        {
            var ws = RED.nodes.workspaces[wsi];
            ws.nodes = new Array();
            for (var ni = 0; ni < RED.nodes.nodes.length; ni++)
            {
                var n = RED.nodes.nodes[ni];
                if (n.z == ws.id)
                {
                    ws.nodes.push(RED.nodes.convertNode(n));
                    //ws.nodes.push(n); // this is only to see structure of raw nodes
                }
            }            
        }

		var settings = {};
		settings["autoSwitchTabToInfo"] =  RED.sidebar.info.autoSwitchTabToThis;
		settings["showWorkspaceToolbar"] = RED.view.showWorkspaceToolbar;
		settings["onlyShowOne"] = RED.palette.onlyShowOne;
		settings["showGridVmajor"] = RED.view.showGridVmajor;
		settings["showGridVminor"] = RED.view.showGridVminor;
		settings["showGridH"] = RED.view.showGridH;
		settings["snapToGrid"] = RED.view.snapToGrid;
		settings["useExportDialog"] = RED.arduino.export.useExportDialog;
		settings["snapToGridXsize"] = RED.view.snapToGridXsize;
		settings["snapToGridYsize"] = RED.view.snapToGridYsize;
        settings["lineCurveScale"] = RED.view.lineCurveScale;
        settings["IOcheckAtExport"] = RED.arduino.export.IOcheckAtExport;
        settings["categoryHeaderTextSize"] = RED.palette.categoryHeaderTextSize;
		var project1 = {}
		project1.settings = settings;
		project1.workspaces = RED.nodes.workspaces;
		//project1.links = RED.nodes.links[0];
		var project_json = JSON.stringify(project1, null, 4);
		RED.arduino.export.showExportDialog("New Project JSON Test", project_json);

		//var project2 = JSON.parse(project_json);
		//console.warn("autoSwitchTabToInfo:" +project2.settings.autoSwitchTabToInfo);
		//console.warn("lineCurveScale:" +project2.settings.lineCurveScale);
	}
    
    return {
        createAndPrintNewWsStruct:createAndPrintNewWsStruct,
        console_logColor:console_logColor
	};
})();