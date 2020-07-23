javac -cp "C:\arduino-1.8.12\lib\pde.jar;C:\arduino-1.8.12\lib\arduino-core.jar;tool\json-20131018.jar" -d bin src\tool_skeleton.java
cd bin
jar cvf Tool_skeleton.jar *
copy Tool_skeleton.jar C:\arduino-1.8.12\tools\Tool_Skeleton\tool\Tool_skeleton.jar
cd ..
pause