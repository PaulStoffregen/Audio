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
        // get all settings that is defined
        var settings = [];
        var RED_Classes = Object.getOwnPropertyNames(RED);
        for (let rci = 0; rci < RED_Classes.length; rci++)
        {
            var RED_Class_Name = RED_Classes[rci];
            var RED_Class = RED[RED_Class_Name];
            var RED_Class_SubClasses = Object.getOwnPropertyNames(RED_Class);

            RED.console_ok("@" + RED_Class_Name);
            //console.log(Object.getOwnPropertyNames(RED_Class));

            for (let i = 0; i < RED_Class_SubClasses.length; i++)
            {
                let RED_SubClass_Name = RED_Class_SubClasses[i];
                if (RED_SubClass_Name == "settings")
                {
                    settings.push({[RED_Class_Name]:RED_Class.settings});
                    //RED.console_ok("found settings@" + RED_Class_Name);
                }
                
            }
        }
        console.warn(RED["arduino"]["export"]);
        console.warn(RED["view"].settings);

		var project1 = {}
		project1.settings = settings;
		project1.workspaces = RED.nodes.workspaces;

		var project_jsonString = JSON.stringify(project1, null, 4);
		RED.arduino.export.showExportDialog("New Project JSON Test", project_jsonString);

        testSettingsLoad(project_jsonString);
    }
    
    function testSettingsLoad(project_jsonString)
    {
        var project = JSON.parse(project_jsonString);
        
        var psettings = project.settings;
        for (let i = 0; i < psettings.length; i++)
        {
            var csettings = psettings[i];
            var json_object = Object.getOwnPropertyNames(csettings)[0]; // there is only one item
            console.log(json_object);
            console.log(csettings[json_object]);
            
            //RED[json_object].settings = csettings[json_object];// this don't run setters 
            var settingValueNames = Object.getOwnPropertyNames(csettings[json_object]);
            console.log("@testSettingLoad:");
            console.log(settingValueNames);
            for (var svi = 0; svi < settingValueNames.length; svi++)
            {
                var valueName = settingValueNames[svi];
                if (RED[json_object].settings[valueName]) // this skip any removed settings
                {
                    RED[json_object].settings[valueName] = csettings[json_object][valueName];
                    console.warn("typeof " + valueName + ":" + typeof csettings[json_object][valueName])
                }
            }
        }
    }
    
    return {
        createAndPrintNewWsStruct:createAndPrintNewWsStruct,
        console_logColor:console_logColor
	};
})();