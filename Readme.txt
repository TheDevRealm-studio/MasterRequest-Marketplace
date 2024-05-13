## Building the Plugin for Production

To build your Unreal Engine plugin for production, follow these steps:

### 1. Copy Plugin Files

Drag the plugin files to the plugins folder in your main project directory.

### 2. Clean Binaries and Intermediate

Delete the `Binaries` and `Intermediate` folders from your project directory.

### 3. Open the Project

Open your Unreal Engine project. The plugin will automatically be compiled and integrated into your project.

For additional information and troubleshooting, refer to the [Unreal Engine Forums](https://forums.unrealengine.com/t/how-to-manually-build-plugins/352910).

### Building Plugin from Command Line

You can also build the plugin from the command line using the following command:

```shell
.\RunUAT.bat BuildPlugin -plugin="C:\Users\mario\Documents\Unreal Projects\MasterRequest\Plugins\MasterRequest\MasterHttpRequest.uplugin" -package="C:\Users\mario\Downloads\Plug" -TargetPlatforms=Win64
```

Replace `"C:\Users\mario\Documents\Unreal Projects\MasterRequest\Plugins\MasterRequest\MasterHttpRequest.uplugin"` with the path to your plugin `.uplugin` file, and `"C:\Users\mario\Downloads\Plug"` with the desired package output path.
