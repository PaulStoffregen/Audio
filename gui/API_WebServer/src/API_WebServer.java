/* -*- mode: java; c-basic-offset: 2; indent-tabs-mode: nil -*- */

/*
  Part of the Processing project - http://processing.org

  Copyright (c) 2008 Ben Fry and Casey Reas

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

package com.API_WebServer;

import java.io.IOException;
import java.io.OutputStream;
import java.io.InputStream;
import java.io.*;
import java.net.InetSocketAddress;

import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpHandler;
import com.sun.net.httpserver.HttpServer;

//import javax.swing.JOptionPane;

import processing.app.Editor;
import processing.app.tools.Tool;
import processing.app.Sketch;
import processing.app.EditorTab;
import processing.app.SketchFile;
import processing.app.EditorHeader;

import java.util.Scanner;

import java.lang.reflect.*;

import java.util.ArrayList;

import static processing.app.I18n.tr;

import javax.swing.JOptionPane;
import javax.swing.JFrame;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;

import java.nio.file.Path;

import org.json.*;


class ANSI{
	public static final String RESET = "\u001B[0m";
	public static final String BLACK = "\u001B[30m";
	public static final String RED = "\u001B[31m";
	public static final String GREEN = "\u001B[32m";
	public static final String YELLOW = "\u001B[33m";
	public static final String BLUE = "\u001B[34m";
	public static final String PURPLE = "\u001B[35m";
	public static final String CYAN = "\u001B[36m";
	public static final String WHITE = "\u001B[37m";
	public static final String BLACK_BACKGROUND = "\u001B[40m";
	public static final String RED_BACKGROUND = "\u001B[41m";
	public static final String GREEN_BACKGROUND = "\u001B[42m";
	public static final String YELLOW_BACKGROUND = "\u001B[43m";
	public static final String BLUE_BACKGROUND = "\u001B[44m";
	public static final String PURPLE_BACKGROUND = "\u001B[45m";
	public static final String CYAN_BACKGROUND = "\u001B[46m";
	public static final String WHITE_BACKGROUND = "\u001B[47m";
}
/**
 * Example Tools menu entry.
 */
public class API_WebServer implements Tool {
	Editor editor;
	
	Sketch sketch; // for the API uses reflection to get
	ArrayList<EditorTab> tabs; // for the API uses reflection to get
	EditorHeader header; // for the API uses reflection to get
	Runnable runHandler; // for the API uses reflection to get
	Runnable presentHandler; // for the API uses reflection to get
	
	HttpServer server;
	int serverPort = 8080;
	
	boolean started = false;

	public void init(Editor editor) {
		this.editor = editor;

		editor.addWindowListener(new WindowAdapter() {
			public void windowOpened(WindowEvent e) {
			  init();
			}
		});
		
	}
	public void run()
	{
		init();
	}
	private void init()
	{
		if (started)
		{
			System.out.println("Server is allready running at port " + serverPort);
			return;
		}
		try{
			Field f = Editor.class.getDeclaredField("sketch");
			f.setAccessible(true);
			sketch = (Sketch) f.get(this.editor);
			
			f = Editor.class.getDeclaredField("tabs");
			f.setAccessible(true);
			
			tabs = (ArrayList<EditorTab>) f.get(this.editor);
			
			f = Editor.class.getDeclaredField("header");
			f.setAccessible(true);
			header = (EditorHeader) f.get(this.editor);
			
			f = Editor.class.getDeclaredField("runHandler");
			f.setAccessible(true);
			runHandler = (Runnable) f.get(this.editor);
			
			f = Editor.class.getDeclaredField("presentHandler");
			f.setAccessible(true);
			presentHandler = (Runnable) f.get(this.editor);
			started = true;
			
		}catch (Exception e)
		{
			sketch = null;
			tabs = null;
			System.err.println("cannot reflect:" + e);
			System.err.println("API_WebServer not started!!!");
			return;
		}
		System.out.println("init API_WebServer");
		LoadSettings();
		startServer();
	}
	
	public void editor_addTab(SketchFile sketchFile, String contents)
	{
		try {
		Method m = Editor.class.getDeclaredMethod("addTab", SketchFile.class, String.class);
		m.setAccessible(true);
		m.invoke(editor, sketchFile, contents);
		}
		catch (Exception e)
		{
			System.err.println("cannot invoke editor_addTab");
		}
	}
	public void sketch_removeFile(SketchFile sketchFile)
	{
		try {
		Method m = Sketch.class.getDeclaredMethod("removeFile", SketchFile.class);
		m.setAccessible(true);
		m.invoke(sketch, sketchFile);
		}
		catch (Exception e)
		{
			System.err.println("cannot invoke sketch_removeFile");
		}
	}
	public void editor_removeTab(SketchFile sketchFile)
	{
		try {
		Method m = Editor.class.getDeclaredMethod("removeTab", SketchFile.class);
		m.setAccessible(true);
		m.invoke(editor, sketchFile);
		}
		catch (Exception e)
		{
			System.err.println("cannot invoke editor_removeTab");
		}
	}
	public boolean sketchFile_delete(SketchFile sketchFile)
	{
		try {
		Method m = SketchFile.class.getDeclaredMethod("delete", Path.class);
		m.setAccessible(true);
		return (boolean)m.invoke(sketchFile, sketch.getBuildPath().toPath());
		}
		catch (Exception e)
		{
			System.err.println("cannot invoke sketchFile_delete" + e);
			return false;
		}
	}
	public boolean sketchFile_fileExists(SketchFile sketchFile)
	{
		try {
		Method m = SketchFile.class.getDeclaredMethod("fileExists");
		m.setAccessible(true);
		return (boolean)m.invoke(sketchFile);
		}
		catch (Exception e)
		{
			System.err.println("cannot invoke sketchFile_fileExists" + e);
			return false;
		}
	}
	
	public boolean addNewFile(String fileName, String contents) // for the API
	{
		File folder;
		try 
		{
			folder = sketch.getFolder();
		}
		catch (Exception e)
		{
			System.err.println(e);
			return false;
		}
		System.out.println("folder: " + folder.toString());
		File newFile = new File(folder, fileName);
		int fileIndex = sketch.findFileIndex(newFile);
		if (fileIndex >= 0) { // file allready exist, just change the contents.
		  tabs.get(fileIndex).setText(contents);
		  System.out.println("file allready exists " + fileName);
		  return true;
		}
		SketchFile sketchFile;
		try {
		  sketchFile = sketch.addFile(fileName);
		} catch (IOException e) {
		  // This does not pass on e, to prevent showing a backtrace for
		  // "normal" errors.
		  System.err.println(e.getMessage());
		  JOptionPane.showMessageDialog(new JFrame(), e.getMessage(), tr("Error"),JOptionPane.ERROR_MESSAGE);
		  
		  return false;
		}
		editor_addTab(sketchFile, contents);
		System.out.println("added new file " + fileName);
		editor.selectTab(editor.findTabIndex(sketchFile));
		
		return true;
	}
	public boolean removeFile(String fileName) // for the API, so that files could be removed
	{
		File newFile = new File(sketch.getFolder(), fileName);
		int fileIndex = sketch.findFileIndex(newFile);
		if (fileIndex >= 0) { // file exist
		    SketchFile sketchFile = sketch.getFile(fileIndex);
			boolean neverSavedTab = !sketchFile_fileExists(sketchFile);
			
			if (!sketchFile_delete(sketchFile) && !neverSavedTab) {
				System.err.println("Couldn't remove the file " + fileName);
				return false;
			}
			if (neverSavedTab) {
				// remove the file from the sketch list
				sketch_removeFile(sketchFile);
			}
			editor_removeTab(sketchFile);
			//try {
				// just set current tab to the main tab
				editor.selectTab(0);

				// update the tabs
				header.repaint();
				return true;
			//} catch (Exception e) {
				// This does not pass on e, to prevent showing a backtrace for
				// "normal" errors.
				//Base.showWarning(tr("Error"), e.getMessage(), null);
			//	JOptionPane.showMessageDialog(new JFrame(), e.getMessage(), tr("Error"),JOptionPane.ERROR_MESSAGE);
			//	System.out.println(e.getMessage());
			//}      
		}
		System.err.println("file don't exists in sketch " + fileName);
		return false;
	}
	public boolean renameFile(String oldFileName, String newFileName) // for the API, so that it can rename files
	{
		File newFile = new File(sketch.getFolder(), oldFileName);
		int fileIndex = sketch.findFileIndex(newFile);
		if (fileIndex >= 0) { // file exist
		  SketchFile sketchFile = sketch.getFile(fileIndex);
		  try {
			sketchFile.renameTo(newFileName);
			// update the tabs
			header.rebuild();
			return true;
		  } catch (IOException e) {
			// This does not pass on e, to prevent showing a backtrace for
			// "normal" errors.
			//Base.showWarning(tr("Error"), e.getMessage(), null);
			JOptionPane.showMessageDialog(new JFrame(), e.getMessage(), tr("Error"),JOptionPane.ERROR_MESSAGE);
			System.err.println(e.getMessage());
		  }
		}
		return false;
	}
	
	public void verifyCompile() 
	{
		editor.handleRun(false, presentHandler, runHandler);
	}
	public void upload()
	{
		editor.handleExport(false);
	}
	public void LoadSettings()
	{
		String filePath = "tools\\API_WebServer\\tool\\settings.json";
		File file = new File(filePath);
		boolean exists = file.exists();
		if (exists)
		{
			
			try {
				String content = new Scanner(file).useDelimiter("\\Z").next();
				JSONObject jsonObj = new JSONObject(content);
				serverPort = jsonObj.getInt("serverPort");
				System.out.println("API_WebServer Setting file loaded: " +filePath);
			} catch (Exception e) {
				System.out.println(e);
				System.out.println("Using default port 8080.");
				serverPort = 8080;
			}
		}
		else
		{
			System.out.println("setting file not found!");
			System.out.println("Using default port 8080.");
			serverPort = 8080;
		}
	}
	
	public String getJSON()
	{
		String filePath = "tools\\API_WebServer\\tool\\settings.json";
		File file = new File(sketch.getFolder(), "GUI_TOOL.json");
		boolean exists = file.exists();
		if (exists)
		{
			
			try {
				String content = new Scanner(file).useDelimiter("\\Z").next();
				return content;
			} catch (Exception e) {
				System.out.println(e);
				return "";
			}
		}
		else
		{
			System.out.println("GUI_TOOL.json file not found!");
			return "";
		}
	}
	
	


	public String getMenuTitle() {
		return "API Web Server";
	}
	
	private void startServer()
	{
		
	  try {
		server = HttpServer.create(new InetSocketAddress("localhost", serverPort), 0);
		server.createContext("/", new  MyHttpHandler(editor, this));
		server.setExecutor(null);
		server.start();
		
		
		System.out.println(" Server started on port " + serverPort);
	  } catch (Exception e) {
		System.out.println(e);
	  }
	}
}
class MyHttpHandler implements HttpHandler
{    
	Editor editor;
	API_WebServer api;

	public MyHttpHandler(Editor _editor, API_WebServer _api)
	{
		this.editor = _editor;
		this.api = _api;
	}
	
	@Override    
	public void handle(HttpExchange httpExchange) throws IOException {
		String reqMethod = httpExchange.getRequestMethod();
		//System.out.println("handle:" + reqMethod);
		String htmlResponse = "OK";
		
		String requestParamValue=null; 
		if("GET".equals(reqMethod))
		{ 
		   //System.out.println("GET");
		   requestParamValue = handleGetRequest(httpExchange);
		   System.out.println("GET request params: " + requestParamValue);
		   if (requestParamValue.equals("compile"))
		   {
			   editor.setAlwaysOnTop(false);
			   editor.setAlwaysOnTop(true);
			   editor.setAlwaysOnTop(false);
			   api.verifyCompile();
		   }
		   else if (requestParamValue.equals("upload"))
		   {
			   editor.setAlwaysOnTop(false);
			   editor.setAlwaysOnTop(true);
			   editor.setAlwaysOnTop(false);
			   api.upload();
		   }
		   else if (requestParamValue.startsWith("renameFile"))
		   {
			   String[] params = requestParamValue.split(":");
			   if (params.length != 3) { System.out.println("Missing parameters @ rename file, length " + params.length + "!=3");  return; }
			   api.renameFile(params[1], params[2]);
		   }
		   else if (requestParamValue.startsWith("removeFile"))
		   {
			   String[] params = requestParamValue.split(":");
			   if (params.length != 2) { System.out.println("Missing parameters @ remove file, length " + params.length + "!=2");  return; }
			   api.removeFile(params[1]);
		   }
		   else if (requestParamValue.startsWith("addFile"))
		   {
			   String[] params = requestParamValue.split(":");
			   if (params.length != 2) { System.out.println("Missing parameters @ add file, length " + params.length + "!=2");  return; }
			   api.addNewFile(params[1], "");
			   editor.handleSave(true);
		   }
		   else if(requestParamValue.equals("getJSON"))
		   {
			   htmlResponse = api.getJSON();
		   }
		   else
			   htmlResponse = "unknown GET method: " + requestParamValue;
		}
		else if("POST".equals(reqMethod))
		{ 
		   requestParamValue = handlePostRequest(httpExchange);
		   ParsePOST_JSON(requestParamValue);
		}

		//System.out.println(requestParamValue); // debug
		handleResponse(httpExchange, htmlResponse); 
	}
	public void ParsePOST_JSON(String data)
	{
		JSONObject jsonObj = new JSONObject(data);
		boolean removeUnusedFiles = jsonObj.getBoolean("removeUnusedFiles"); // this should be implemented later
		JSONArray arr = jsonObj.getJSONArray("files");
		
		for (int i = 0; i < arr.length(); i++)
		{
			JSONObject e = arr.getJSONObject(i);
			String name = e.getString("name");
			//if (!name.endsWith(".h"))
			//	name += ".h";
			String cpp = e.getString("cpp");
			//editor.addNewFile(name, cpp); // need modificzation of arduino IDE source code
			
			api.addNewFile(name, cpp); // uses reflection to use private members
			//TODO: fix so that tabs/files that allready exist in the sketch is removed if they not exist in the json, this should be optional
		}
		//System.out.println(data);
		editor.handleSave(true);
	}
	
	private String handleGetRequest(HttpExchange httpExchange) {
		httpExchange.getResponseHeaders().add("Access-Control-Allow-Origin", "*");
		//System.out.println("handleGetRequest uri:" + httpExchange.getRequestURI());
		return httpExchange.
			getRequestURI()
			.toString()
			.split("\\?")[1]
			.split("=")[1];
	}
	
	private String handlePostRequest(HttpExchange httpExchange) {
		httpExchange.getResponseHeaders().add("Access-Control-Allow-Origin", "*");
		if (httpExchange.getRequestMethod().equalsIgnoreCase("OPTIONS")) {
            httpExchange.getResponseHeaders().add("Access-Control-Allow-Methods", "GET, OPTIONS");
            httpExchange.getResponseHeaders().add("Access-Control-Allow-Headers", "Content-Type,Authorization");
            try{
			httpExchange.sendResponseHeaders(200, 0);
			} catch (Exception e) {
			System.out.println(e);
			}
			System.out.println("hi");
            return "";
        }
		InputStream input = httpExchange.getRequestBody();
        StringBuilder stringBuilder = new StringBuilder();

        new BufferedReader(new InputStreamReader(input))
                          .lines()
                          .forEach( (String s) -> stringBuilder.append(s + "\n") );

		return stringBuilder.toString();
	}

	private void handleResponse(HttpExchange httpExchange, String htmlResponse)  throws  IOException {
		OutputStream outputStream = httpExchange.getResponseBody();
		/*StringBuilder htmlBuilder = new StringBuilder();

		htmlBuilder.append("<html>")
					.append("<body>")
					.append("<h1>")
					.append("OK")
					.append("</h1>")
					.append("</body>")
					.append("</html>");

		String htmlResponse = htmlBuilder.toString();*/
		//System.out.println("htmlResponse:"+htmlResponse);
	  
		// this line is a must
		httpExchange.sendResponseHeaders(200, htmlResponse.length());
		// additional data to send back
		outputStream.write(htmlResponse.getBytes());
		outputStream.flush();
		outputStream.close();
	}
}