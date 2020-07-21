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
RED.settings = (function() {

    function createTab()
	{
	/*	app.UseCors(builder => builder
					.AllowAnyOrigin()
					.AllowAnyMethod()
					.AllowAnyHeader());*/

		var content = document.createElement("div");
		content.id = "tab-settings";
		content.style.paddingTop = "4px";
		content.style.paddingLeft = "4px";
		content.style.paddingRight = "4px";
		RED.sidebar.addTab("settings",content);
        var html = "<h3>Settings</h3>";
        html += createCheckBox("setting-auto-switch-to-info-tab", "Auto switch to info-tab when selecting node(s).");
		html += createCheckBox("setting-show-workspace-toolbar", "Show Workspace toolbar.");
		html += createCheckBox("setting-show-palette-onlyOne", "Palette Show Only one category at a time.");
		html += createCheckBox("setting-show-workspace-v-grid", "Show Workspace v-grid.");
		html += createCheckBox("setting-show-workspace-h-grid", "Show Workspace h-grid.");
		html += createCheckBox("setting-snap-to-grid", "Snap to grid.");
		//html += createTextInputWithApplyButton("setting-grid-xSize", "Grid Size X");
		//html += createTextInputWithApplyButton("setting-grid-ySize", "Grid Size Y");
		html += createTextInputWithApplyButton("setting-test-post", "test post");
		html += createTextInputWithApplyButton("setting-test-get", "test get");

		//html += '<p><br><br>this is a placeholder for future settings:<br>accessible with:<br>$("#tab-settings").html("text");</p>';

		$("#tab-settings").html(html);
        functionalizeCheckBox("setting-auto-switch-to-info-tab", RED.sidebar.info.autoSwitchTabToThis, RED.sidebar.info.setAutoSwitchTab);
		functionalizeCheckBox("setting-show-workspace-toolbar", RED.view.showWorkspaceToolbar, RED.view.setShowWorkspaceToolbarVisible);
		functionalizeCheckBox("setting-show-palette-onlyOne", RED.palette.onlyShowOne, RED.palette.SetOnlyShowOne);
		functionalizeCheckBox("setting-show-workspace-v-grid", RED.view.showGridV, RED.view.showHideGridV);
		functionalizeCheckBox("setting-show-workspace-h-grid", RED.view.showGridH, RED.view.showHideGridH);
		functionalizeCheckBox("setting-snap-to-grid", RED.view.snapToGrid, RED.view.setSnapToGrid);
		//functionalizeTextInputWithApplyButton("setting-grid-xSize", 500, function(value) { console.error("new grid-xsize:" + value);});
		//functionalizeTextInputWithApplyButton("setting-grid-ySize", 500, function(value) { console.error("new grid-ysize:" + value);});
		functionalizeTextInputWithApplyButton("setting-test-post", "Hello", RED.arduino.httpPostAsync);
		functionalizeTextInputWithApplyButton("setting-test-get", "cmd", RED.arduino.httpGetAsync);

		RED.sidebar.show("settings"); // development, allways show settings tab first
	}
	
	/**
	 * creates and returns html code for a checkbox with label
	 * @param {string} id 
	 * @param {string} label 
	 */
	function createCheckBox(id, label)
	{
		var html = '<label for="'+id+'" style="font-size: 16px; padding: 2px 0px 0px 4px;">';
		html +=	'<input style="margin-bottom: 4px; margin-left: 4px;" type="checkbox" id="'+id+'" checked="checked" />';
		html +=	'&nbsp;'+label+'</label>';
		return html;
	}
	function createTextInputWithApplyButton(id, label)
	{
		var html = '<label for="'+id+'" style="font-size: 16px;">';
			html += '&nbsp;'+label+' <input type="text" id="'+id+'" name="'+id+'" style="width: 40px;">';
			html += ' <button type="button" id="btn-'+id+'">Apply</button></label>';
		return html;
	}
	function functionalizeTextInputWithApplyButton(id, text, func)
	{
		$('#btn-' + id).click(function() { func($('#' + id).val());});
		$('#' + id).val(text);
	}
	/**
	 * this must be run after the html is applied to the document
	 * @param {string} id 
	 * @param {boolean} initalState 
	 * @param {function} func 
	 */
	function functionalizeCheckBox(id, initalState, func)
	{
		$('#' + id).click(function() { func($('#' + id).prop('checked'));});
		$('#' + id).prop('checked', initalState);
    }
    
    return {
		createTab:createTab,
	};
})();
