To build for production,

1:Drag the files to the plugins folder in your main project
2: delete the Binaries and Intermediate
3: open the project and that is it

mor info:
https://forums.unrealengine.com/t/how-to-manually-build-plugins/352910

Code compile:
.\RunUAT.bat BuildPlugin -plugin="C:\Users\mario\Documents\Unreal Projects\MasterRequest\Plugins\MasterRequest\MasterHttpRequest.uplugin" -package="C:\Users\mario\Downloads\Plug" -TargetPlatforms=Win64
