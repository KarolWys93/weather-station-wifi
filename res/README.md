# Images

The E-Paper display used in the device has a resolution of 200x200 pixels and supports up to 5 colors: black, white, red, gray and pink. The display driver uses 3 "images" for black, gray and pink. A red color pixel is obtained when the same pixel is set for a black and pink "image".

## Image list

Here is the list of images used in system. Every image has its own index:

- 0 - memory error image *
- 1 - forecast background image *
- 2 - configuration mode image
- 3 - Low battery image
- 4 - wifi error image
- 5 - general error image

Memory error and forecast background images are loaded from the FLASH memory. The rest of the images are stored on the SD memory card. Images on the SD card are stored in **/img/** directory as epd files. The file name format is:
***/img/x_y.epd***
Where *x* is index and *y* is color. Example:
*/img/2_b.epd  
/img/2_g.epd  
/img/2_r.epd*

The example shows configuration mode images for black, grey and red display colors. It isn't mandatory to provide epd file if it is blank.

## Creating and converting images

Images should have a resolution of 200x200 pixels and use only the colors listed:
0x000000 : black  
0xED1C24 : red  
0xC3C3C3 : grey  
0xEF88BE : pink  
0xFFFFFF : white

Prepared image have to be split into monochrome bitmaps using script ***color_split.py***. Example:

`python res/scripts/color_split.py res/low_bat.bmp`

The result is 3 monochrome bitmaps:
*low_bat_b.bmp  
low_bat_g.bmp  
low_bat_r.bmp*

### Converting to file
Now these bitmaps should be converted to edp files. edp file is a raw image buffer for displaying e-paper.
The conversion can be done using the script ***epd_conv.py***. Example:

`python res/scripts/epd_conv.py res/low_bat_b.bmp`

The result is *low_bat_b.epd* file.
Now the file can be moved to the SD card (and renamed respectively).

### Converting to code
If image should be stored in FLASH memory, then image file can be converted to code (text). Example:

`python res/scripts/epd_conv.py -t res/low_bat_b.bmp`

The output is a text file. The content of the file should be copy to source code as array.

## "Screenshots" and back conversion

For debugging purposes, display buffers can be dumped to files. The dumps are saved as epd files in ***/img/scr*** directory.
The epd files can be converted to image using script ***scr2img.py***. Example:

`python res/scripts/scr2img.py b.epd g.epd r.epd output_img.bmp`

## Dependences

All the mentioned python scripts require the numpy and PIL libraries.
