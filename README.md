# PPM HDR Viewer

This solutionn contains two projects:
* **HDR Viewer**: This allows for 16-bit `*.ppm` images to be displayed with options for stereo and flickering
* **PPM Experiment**: Displays specific regions of a set of `*.ppm` images on two monitors, configured in a `*.json` file.

## HDR Viewer

Allows viewing of steroscopic 16 bit HDR PPM images via OpenCV + DirectXTK, with options for stereo viewing and flicker testing.
 
### Installation

1. Build and install OpenCV and my `DirectXTK_Desktop_2019_Win10` fork (available [here](https://github.com/richardrobinson0924/DirectXTK)) using `x64`
2. Set the environment variables `DirectXTK_Path` to `...\DirectXTK` and `OpenCV_Path` to `...\opencv\build`
4. Build and compile the project (using `x64`).

### Requirements

- Windows 10, Fall Creators Update 2018 or later
- NVIDIA GPU
- 4K HDR (10 bit) capable monitor(s)
- OpenCV v4.0+, DirectX 11, DirectXTK fork

### Options

**Option 1**: Double click a 4K 16-bit `*.ppm` image to view it. (You may have to right click on the file > `Open With` annd select `HDRViewer19.exe`)

**Option 2**: Run `HDRViewer19.exe`. In the configuration wizard, you may select several options:
* *Stereo*: Allows viewing of stereoscopic images. Requires two monitors be connected. There must be at least 2 permutations of each image, or 4 if *Flicker* is also enabled.
* *Flicker*: Flickers the image at a specific rate. There must be at least 2 permutations of each image, or 4 if *Stereo* is also enabled.

Then, select the directory which contains the images.

### Usage
Use the arrow keys to navigate between the images. Press `Esc` to quit the app.


### Credits

Created by Richard Robinson under The Centre for Vision Research at York University, Toronto, Canada.
