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

package com.tool_skeleton;

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

import org.json.*;

/**
 * Example Tools menu entry.
 */
public class Tool_Skeleton implements Tool {
	Editor editor;
	HttpServer server;
	

	public void init(Editor editor) {
		this.editor = editor;
		System.out.println("init tool_skeleton");
		startServer();
	}


	public String getMenuTitle() {
		return "Start Teensy Audio GUI Server";
	}
	
	private void startServer()
	{
	  try {
		server = HttpServer.create(new InetSocketAddress("localhost", 8080), 0);
		server.createContext("/", new  MyHttpHandler(editor));
		server.setExecutor(null);
		server.start();
		
		
		System.out.println(" Server started on port 8080");
	  } catch (Exception e) {
		System.out.println(e);
	  }
	}


	public void run() {

	}
	
	

}
class MyHttpHandler implements HttpHandler
{    
	Editor editor;
	
	Runnable runHandler;
	Runnable presentHandler;
	
	private void resetHandlers()
	{
		//runHandler = new Editor.BuildHandler();
		//presentHandler = new Editor.BuildHandler(true);
	}
	public MyHttpHandler(Editor _editor)
	{
		this.editor = _editor;
		resetHandlers();
		
	}
	
	@Override    
	public void handle(HttpExchange httpExchange) throws IOException {
		String reqMethod = httpExchange.getRequestMethod();
		System.out.println("handle:" + reqMethod);
		
		String requestParamValue=null; 
		if("GET".equals(reqMethod))
		{ 
		   System.out.println("GET");
		   requestParamValue = handleGetRequest(httpExchange);
		   System.out.println(requestParamValue);
		   if (requestParamValue.equals("compile"))
		   {
			   editor.setAlwaysOnTop(false);
			   editor.setAlwaysOnTop(true);
			   editor.setAlwaysOnTop(false);
			   editor.verifyCompile();
		   }
		   else if (requestParamValue.equals("upload"))
		   {
			   editor.setAlwaysOnTop(false);
			   editor.setAlwaysOnTop(true);
			   editor.setAlwaysOnTop(false);
			   editor.upload();
		   }
		}
		else if("POST".equals(reqMethod))
		{ 
		   requestParamValue = handlePostRequest(httpExchange);
		   ParsePOST_JSON(requestParamValue);
		   
		   //editor.addNewFile("GUIgenerated.h", requestParamValue);
			//editor.getCurrentTab().setText(requestParamValue);
		}

		//System.out.println(requestParamValue); // debug
		handleResponse(httpExchange); 
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
			if (!name.endsWith(".h"))
				name += ".h";
			String cpp = e.getString("cpp");
			editor.addNewFile(name, cpp);
			//TODO: fix so that tabs/files that allready exist in the sketch is removed if they not exist in the json, this should be optional
		}
	}
	private String handleGetRequest(HttpExchange httpExchange) {
		System.out.println("handleGetRequest uri:" + httpExchange.getRequestURI());
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

	private void handleResponse(HttpExchange httpExchange)  throws  IOException {
		OutputStream outputStream = httpExchange.getResponseBody();
		StringBuilder htmlBuilder = new StringBuilder();

		htmlBuilder.append("<html>")
					.append("<body>")
					.append("<h1>")
					.append("OK")
					.append("</h1>")
					.append("</body>")
					.append("</html>");

		String htmlResponse = htmlBuilder.toString();
		//System.out.println("htmlResponse:"+htmlResponse);
	  
		// this line is a must
		httpExchange.sendResponseHeaders(200, htmlResponse.length());
		// additional data to send back
		outputStream.write(htmlResponse.getBytes());
		outputStream.flush();
		outputStream.close();
	}
}