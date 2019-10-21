# dominant-colors-rgb-wheel
## Find dominant colors in images with QT and OpenCV, with a nice GUI to show results in 3D color spaces
### v0 - 2019-10-21

![Screenshot - Global](screenshots/screenshot-gui.jpg?raw=true)
<br/>

## HISTORY

* v0: launch
<br/>
<br/>

## LICENSE

The present code is under GPL v3 license, that means you can do almost whatever you want with it!

I used bits of code from several sources, which are cited within the code
<br/>
<br/>

## WHY?

Lately I paid great attention to colors in my photo editing process. Delicately tuning colors can greatly improve an already good photo, to make it stunning!

I tried to find tools on the internet, but none of them had all I wanted, so I started coding once again. The first version was only a color wheel... now you can visualize the results in full 3D, in the most-known color spaces.

I'm not an ace of C++ and QT. So, if you don't find my code pretty never mind, because it WORKS, and that's all I'm asking of it :)
<br/>
<br/>

## WITH WHAT?

Developed using:
* Linux Ubuntu	16.04
* QT Creator 3.5
* Requires these libraries:
  * QT 5
  * openCV 4.1 compiled with openCV-contribs - should work with 3.x versions without much editing
  * OpenGL - I used deprecated functions starting from v3.1, but it works under Ubuntu with my GeForce 1080 Ti

This software should also work under Microsoft Windows: if you tried it successfully please contact me, I'd like to offer compiled Windows executables
<br/>
<br/>

## HOW?

* Help is available within the GUI, with each element's tooltip. Just hover your mouse
* All is on one window, the tool is pretty simple once acquainted

### IMAGE

![Screenshot - Image](screenshots/screenshot-image.jpg?raw=true)

* Just click the "Load image" button

* You have two options before loading:
	* Reduce size: the biggest the image, the longest you wait! Tests have shown that reducing the image to 512 pixels doesn't affect much the dominant colors distribution. It also helps with noisy images
	* Gaussian blur: you might not want to reduce the image, but image noise can affect results VS what you really perceive. The solution is to apply a 3x3 Gaussian blur that helps smooth surfaces

### FINDING DOMINANT COLORS

![Screenshot - Compute](screenshots/screenshot-compute.jpg?raw=true)

* How many dominant colors do you want? Choose wisely, bigger values take greater time to compute (you have a timer but not in real-time)

* You have to choose the algorithm first. Two are at your service:
	* Eigen vectors: the fastest - source: http://aishack.in/tutorials/dominant-color/
	* K-means: a well-known algorithm to aggregate significant data  - source: https://jeanvitor.com/k-means-image-segmentation-opencv/

* Click Compute to finish: you end up with 3D view, a quantized image and a palette. The elapsed time is shown in the LCD display

### ACCURACY OF ALGORITHMS

![Screenshot - Accuracy](screenshots/joconde-quantized.jpg?raw=true)

How efficient are the two algorithms ? To check, I just had to test my tool on a RGB palette I produced in 5 minutes with Photoshop with the exact RGB values of the 12 Primary to Tertiary colors.

The two methods seem pretty good at first glance. Even more when you click on the palette: a perfect match for both! The difference between the two methods will only be seen with complex images.

### PALETTE

![Screenshot - Palette](screenshots/screenshot-palette.jpg?raw=true)

* The palette shows all the dominant colors, and their proportion in the Quantized image. Don't set the number of colors over 250 if you want quality, the palette image is only 1000 pixels long

* The Quantized image shows the image transposed to the dominant colors Palette. This way you can visually check if the number of dominant colors chosen at the beginning is sufficient or too wide

* Use the Sort selector to organize colors in several ways, using the values from the color spaces

* You can left-click with your mouse on any color on the Quantized image or the Palette, to get information:
	* RGB values of picked color, in decimal and hexadecimal
	* percentage of use in the quantized image
	* color name. Sometimes it is poetic, and sometimes it is just a code. This information was tricky to develop:
		* I used a text file containing more than 9000 RGB values and corresponding color names from http://mkweb.bcgsc.ca/colornames
		* if the exact RGB value is found the name is displayed, if not the nearest color is displayed (using the euclidian distance in RGB color space between the two colors)
		* remember to put color-names.txt in the same folder as the executable (particularly in your compiling folder)

### 3D VIEW

![Screenshot - 3D](screenshots/screenshot-3d.jpg?raw=true)

* The 3D view is where the dominant colors of the image are now displayed:
	* it shows the dominant colors, so it can help you analyze how a photo or painting colors are distributed
	* you can easily visualize / understand the color distribution: the sphere size of a dominant color is the percentage of this color in the whole image
	* you have several color spaces in which to display the colors, each has its pros and cons
	* you can easily navigate with your mouse + keyboard

* Controls :
	* to use the mouse in the 3D space, be sure the focus is on it, not in a combo box fox example
	* left mouse button: hold and move your mouse to rotate the view on the x and y axes (you can also use page-up, page-down, home and end keys, or the big sliders, or the combox boxes near the sliders)
	* CTRL + left mouse button: hold and move left-right to rotate the view on the z axis (you can also use insert and delete keys)
	* right mouse button: drag to move the view on the x and y axes (you can also use left, right, up and down keys)
	* the Reset button resets the zoom, center x and y, and x y and z rotation values to something that fits the current color space well
	* the Full screen button makes the 3D view... full screen! Use the ESC key, it is the only way to get out of here. A button to save the current view is in the top-left corner
	
* Light: gives more depth (shadows and light) to the spheres representing the colors - it is prettier BUT the colors become non-accurate. Use the Light switch to turn light on and off

### SAVING THE RESULTS

![Screenshot - Saving](screenshots/screenshot-buttons.jpg?raw=true)

* If you want to save the results, click on the "Save results" button. They will be saved with the provided file name + suffixes:
	* Palette: filename-palette.png
	* Color space: filename-color-space-XXX.png
	* Quantized image: filename--quantized.png
	* CSV file of palette: filename-palette.csv - RGB values (decimal and hexadecimal) + all known color space values + percentage are saved - you can easily import it with LibreOffice Calc!

![Screenshot - CSV](screenshots/screenshot-csv.jpg?raw=true)
	
### COLOR SPACES

![Screenshot - Color Spaces](screenshots/screenshot-color-space.jpg?raw=true)
	
* The hardest part to code! Lots of maths, lots of trial-and-error, but the results are pretty cool :)

* I am not sure that what I coded about color spaces is absolutely right: if you find errors or misunderstandings, please let me know

* You can visualize the dominant colors palette in any of the following color spaces:
(examples below show the Joconda dominant colors distribution)

	* RGB: just a cube with Red, Green and Blue as axes. Simple to understand, all the details here: https://en.wikipedia.org/wiki/RGB_color_space
![Screenshot - RGB](screenshots/joconde-in-color-space-rgb.jpg?raw=true)
	* HSV, HSL, HCV, HCL: these spaces are based on Hue. Then you have several methods, using several values: Lightness, Value, Chroma, and Saturation. The global shape of the space is a cylinder or cone. More here: https://en.wikipedia.org/wiki/HSL_and_HSV
![Screenshot - HSL](screenshots/example-color-space-hsl.jpg?raw=true)
	* HWB: more simple to understand than HSV or HSL, HWB is also based on Hue, but with a little White and Black added. The space shape is conical. See here: https://en.wikipedia.org/wiki/HWB_color_model
![Screenshot - HWB](screenshots/example-color-space-hwb.jpg?raw=true)
	* CIE XYZ: an imaginary color space, which I chose to represent with the famous horse-shoe colored shape. Be sure to put the file xyz-space.csv with the executable, it contains all the coodinates of the boundaries, each corresponding to a light wavelength. The only problem is that a dark and lighter version of the same color can be on the same spot, because the horse-shoe is almost on one plane. XYZ is the basis of several CIE color spaces
![Screenshot - XYZ](screenshots/example-color-space-xyz.jpg?raw=true)
	* CIE L*A*B*: directly based on XYZ, it uses a white point as reference, mine is 2° and 65K, that's what you will see with the 3D view. LAB is largely used, for example in Photoshop. It is pretty easy to understand and visualize. CIE XYZ and LAB information here: https://en.wikipedia.org/wiki/CIE_1931_color_space
![Screenshot - L*A*B*](screenshots/example-color-space-lab.jpg?raw=true)
	* Color Wheel: the good ol' one, already used in my previous tool, but this time in 3D, even if's only on one plane. This one is nice to find color correlations like "complementary" or "tetradric", etc. General info there: https://en.wikipedia.org/wiki/Color_wheel
![Screenshot - Wheel](screenshots/example-color-space-wheel.jpg?raw=true)

* When choosing a color space, don't forget you have "default" viewing configurations, just use the "Reset" button

* The slider under the color spaces selector is used to control the color spheres size

* The last button is to save a snapshot of the curent 3D view. It also appears in full screen mode

<br/>
<br/>

## Enjoy!

### AbsurdePhoton
My photographer website ''Photongénique'': www.absurdephoton.fr
