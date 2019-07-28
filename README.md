# PPM HDR Viewer

Allows viewing of steroscopic 10 bit HDR PPM images via OpenCV + DirectXTK, with options for stereo viewing and flicker testing.
 
### Installation

1. Build and install OpenCV and my `DirectXTK_Desktop_2019_Win10` fork (available [here](https://github.com/richardrobinson0924/DirectXTK)) using `x64`
2. Set the environment variables `DirectXTK_Path` to `...\DirectXTK` and `OpenCV_Path` to `...\opencv\build`
4. Build and compile the project (using `x64`).

### Requirements

- Windows 10, Fall Creators Update 2018 or later
- NVIDIA GPU
- 2 4K HDR (10 bit) capable monitors
- At least four permutations of a single 4K 10 bit PPM image
- OpenCV v4.0+, DirectX 11, DirectXTK fork

### Usage

In Command Prompt: 

```
HDRViewer19.exe [-flicker] path
```

- `flicker`: flickers between the first and the third permutations of an image at a rate of 100ms
- `path`: the absolute path to the directory containing the images. It is required that there are four (alphabetically) consecutive permutations of each image in the folder, even if `flicker` and `stereo` are disabled.

Press the space bar to cycle through the images in `path`. To quit the app, press `Esc`

### Credits

Created by Richard Robinson under The Centre for Vision Research at York University, Toronto, Canada.
