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
		var htmlCategory = "";
        html += createCheckBox("setting-auto-switch-to-info-tab", "Auto switch to info-tab when selecting node(s).");
		html += createCheckBox("setting-show-workspace-toolbar", "Show Workspace toolbar.");
		html += createCheckBox("setting-show-palette-onlyOne", "Palette Show Only one category at a time.");
		htmlCategory = "";
		htmlCategory += createCheckBox("setting-show-workspace-v-grid-ma", "Show Workspace major v-grid.");
		htmlCategory += createCheckBox("setting-show-workspace-v-grid-mi", "Show Workspace minor v-grid.");
		htmlCategory += createCheckBox("setting-show-workspace-h-grid", "Show Workspace h-grid.");
		htmlCategory += createCheckBox("setting-snap-to-grid", "Snap to grid.");
		htmlCategory += createTextInputWithApplyButton("setting-snap-grid-xSize", "Snap Grid Size X");
		htmlCategory += createTextInputWithApplyButton("setting-snap-grid-ySize", "Snap Grid Size Y");
		html += createCategory("workspace-grid", "Workspace Grid", htmlCategory);

		htmlCategory = "";
		htmlCategory += createTextInputWithApplyButton("setting-test-post", "test post");
		htmlCategory += createTextInputWithApplyButton("setting-test-get", "test get");
		html += createCategory("development-tests", "Development Tests", htmlCategory);

		$("#tab-settings").html(html);
		
		// theese functionalize functions allways end with the callback function
        functionalizeCheckBox("setting-auto-switch-to-info-tab", RED.sidebar.info.autoSwitchTabToThis, RED.sidebar.info.setAutoSwitchTab);
		functionalizeCheckBox("setting-show-workspace-toolbar", RED.view.showWorkspaceToolbar, RED.view.setShowWorkspaceToolbarVisible);
		functionalizeCheckBox("setting-show-palette-onlyOne", RED.palette.onlyShowOne, RED.palette.SetOnlyShowOne);
		functionalizeCheckBox("setting-show-workspace-v-grid-ma", RED.view.showGridVmajor, RED.view.showHideGridVmajor);
		functionalizeCheckBox("setting-show-workspace-v-grid-mi", RED.view.showGridVminor, RED.view.showHideGridVminor);
		functionalizeCheckBox("setting-show-workspace-h-grid", RED.view.showGridH, RED.view.showHideGridH);
		functionalizeCheckBox("setting-snap-to-grid", RED.view.snapToGrid, RED.view.setSnapToGrid);
		functionalizeTextInputWithApplyButton("setting-snap-grid-xSize", RED.view.snapToGridXsize, RED.view.setSnapToGridXsize);
		functionalizeTextInputWithApplyButton("setting-snap-grid-ySize", RED.view.snapToGridYsize, RED.view.setSnapToGridYsize);
		functionalizeTextInputWithApplyButton("setting-test-post", "Hello", RED.arduino.httpPostAsync);
		functionalizeTextInputWithApplyButton("setting-test-get", "cmd", RED.arduino.httpGetAsync);
		functionalizeCategory("workspace-grid");
		functionalizeCategory("development-tests");
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
	
	function createCategory(category, header, content)
	{
		var chevron = '<i class="icon-chevron-down"></i>';
		var displayStyle = "none";
		var html = '<div class="palette-category">'+ // use style from palette-category
			'<div id="setting-header-'+category+'" class="palette-header">'+chevron+'<span>'+header+'</span></div>'+
			'<div class="palette-content" id="palette-base-category-'+category+'" style="display: '+displayStyle+';">'+
			  content+
			'</div>'+
			'</div>'
		return html;
	}
	function functionalizeCategory(category)
	{
		$("#setting-header-"+category).off('click').on('click', function(e) {
			
			var displayStyle = $(this).next().css('display');
			if (displayStyle == "block")
			{
				$(this).next().slideUp();
				$(this).children("i").removeClass("expanded"); // chevron
			}
			else
			{
				$(this).next().slideDown();
				$(this).children("i").addClass("expanded"); // chevron
			}
		});
	}
    
    return {
		createTab:createTab,
	};
})();
