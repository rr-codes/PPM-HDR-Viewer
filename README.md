# PPM HDR Viewer

This solutionn contains two projects:
* **HDR Viewer**: This allows for 16-bit `*.ppm` images to be displayed with options for stereo and flickering
* **PPM Experiment**: Displays specific regions of a set of `*.ppm` images on two monitors, configured in a `*.json` file.

### Installation

1. Build and install OpenCV and my `DirectXTK_Desktop_2019_Win10` fork (available [here](https://github.com/richardrobinson0924/DirectXTK)) using `x64`
2. Set the environment variables `DirectXTK_Path` to `...\DirectXTK` and `OpenCV_Path` to `...\opencv\build`
4. Build and compile the project (using `x64`).

### Requirements

- Windows 10, Fall Creators Update 2018 or later
- NVIDIA GPU
- 4K HDR (10 bit) capable monitor(s)
- OpenCV v4.0+, DirectX 11, DirectXTK fork

## HDR Viewer

Allows viewing of steroscopic 16 bit HDR PPM images via OpenCV + DirectXTK, with options for stereo viewing and flicker testing.

### Options

**Option 1**: Double click a 4K 16-bit `*.ppm` image to view it. (You may have to right click on the file > `Open With` annd select `HDRViewer19.exe`)

**Option 2**: Run `HDRViewer19.exe`. In the configuration wizard, you may select several options:
* *Stereo*: Allows viewing of stereoscopic images. Requires two monitors be connected. There must be at least 2 permutations of each image, or 4 if *Flicker* is also enabled.
* *Flicker*: Flickers the image at a specific rate. There must be at least 2 permutations of each image, or 4 if *Stereo* is also enabled.

Then, select the directory which contains the images.

### Usage
Use the arrow keys to navigate between the images. Press `Esc` to quit the app.


----

## PPM Experiment

### Setup
This experiment is designed based on the ISO Flicker paradigm to evaluate different compression codecs and levels. The experimental app must be run on Windows 10, connected to two 4K HDR-10 monitors on a single GPU.

All images must be within a single directory. Each image should have four permutations:
- `_L_orig`: The original image for the left display
- `_L_dec`: The decompressed image for the left display
- `_R_orig`: The original image for the right display
- `_L_dec`: The decompressed image for the right display

An XBOX controller or the arrow keys may be used.

### Usage
1. Upon launch, select the configuration `json` file you wish to use. The file should have the following items:
 ```json
 {
    "Participant ID" : "Richard",
    "Image Directory" : "C://",
    "Trials" : [
       {
          "Image Name": "something",
          "Correct Side" : 1,
          "Position" : {
             "x" : 0,
             "y" : 0
          },
          "Viewing Mode" : 2
       }
    ]
 }
 ```
 (_Correct Side_: For left, indicate `1`. For right, `2`.
 _Viewing Mode_: For stereo, indicate `0`. For mono l/r, `1`/`2`)

2. When the launch screen displays, press `A` on the game pad, or `Enter` on the keyboard to start.
3. For each image `Image Name`, each display will present 2 images, cropped to begin at `Position` and whose dimensions are indicated by `Image Dimensions`, and seperated bt `Distance`. The image on the side of `Correct Side` will flicker between the original and decompressed permutations at a rate of `Flicker Rate`, and the other image will display the original permutation.
4. The `Viewing Mode` parameter indicates if both sides should display the left or right images, or their respective images.
5. Select `L` or `R` on the game pad (or `<-, ->` on a keyboard) to indicate which image appears to be flickering. If, after `Timeout Duration` seconds, an answer has not been indicated, the images will dissapear until answered.
6. If correct, a success tone will sound.


### Credits

Created by Richard Robinson under The Centre for Vision Research at York University, Toronto, Canada.
