# PPM HDR Viewer

Allows viewing of 10 bit HDR PPM images, with options for stereo viewing and flicker testing.
 
**Installation**:

1. Build and install OpenCV and DirectXTK Desktop 2015
2. Open this project in Visual Studio. Go to `Solution Explorer > HDRViewr19 > Properties > C/C++ > General`, and change the paths of DirectXTK and OpenCV to their respective `include` directory paths. 
3. Repeat for the library `lib` directories in `Linker > Additional Library Directories`
4. Build and compile the project.

**Requirements**:

- Windows 10, Fall Creators Update 2018 or later
- NVIDIA GPU
- A 4K HDR (10 bit) capable monitor.

Full screen mode is required for proper viewing.

**Usage**:

In Command Prompt: `HDRViewer19.exe [-flicker] [-stereo] path`

- `flicker`: flickers between the first and the third permutations of an image at a rate of 100ms
- `stereo`: opens two windows containing the first and second permutations of an image. If both `stereo` and `flicker` are enabled, the second and fourth permutations flicker between themselves.
- `path`: the absolute path to the directory containing the images. It is required that there are four (alphabetically) consecutive permutations of each image in the folder, even if `flicker` and `stereo` are disabled.

To enter or exit full screen mode, click `Alt + Enter`
