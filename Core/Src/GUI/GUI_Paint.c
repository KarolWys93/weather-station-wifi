/*****************************************************************************
* | File      	:   GUI_Paint.c
* | Author      :   Waveshare team
* | Function    :	Achieve drawing: draw points, lines, boxes, circles and
*                   their size, solid dotted line, solid rectangle hollow
*                   rectangle, solid circle hollow circle.
* | Info        :
*   Achieve display characters: Display a single character, string, number
*   Achieve time display: adaptive size display time minutes and seconds
*----------------
* |	This version:   V2.0
* | Date        :   2018-11-15
* | Info        :
* 1.add: Paint_NewImage()
*    Create an image's properties
* 2.add: Paint_SelectImage()
*    Select the picture to be drawn
* 3.add: Paint_SetRotate()
*    Set the direction of the cache    
* 4.add: Paint_RotateImage() 
*    Can flip the picture, Support 0-360 degrees, 
*    but only 90.180.270 rotation is better
* 4.add: Paint_SetMirroring() 
*    Can Mirroring the picture, horizontal, vertical, origin
* 5.add: Paint_DrawString_CN() 
*    Can display Chinese(GB1312)    
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documnetation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to  whom the Software is
* furished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*
******************************************************************************/
#include "e-Paper/Config/Debug.h"
#include "e-Paper/Config/DEV_Config.h"
#include "GUI/GUI_Paint.h"
#include <stdint.h>
#include <stdlib.h>
#include "dma.h"
#include <math.h>

PAINT Paint;
/******************************************************************************
function:	Create Image
parameter:
    image   :   Pointer to the image cache
    width   :   The width of the picture
    Height  :   The height of the picture
    Color   :   Whether the picture is inverted
******************************************************************************/
void Paint_NewImage(UBYTE *image, UWORD Width, UWORD Height, UWORD Rotate, UWORD Color)
{
    Paint.Image = NULL;
    Paint.Image = image;

    Paint.WidthMemory = Width;
    Paint.HeightMemory = Height;
    Paint.Color = Color;
    Paint.WidthByte = (Width % 8 == 0)? (Width / 8 ): (Width / 8 + 1);
    Paint.HeightByte = Height;
    Debug("WidthByte = %d, HeightByte = %d", Paint.WidthByte, Paint.HeightByte);
    Debug("EPD_WIDTH / 8 = %d",  122 / 8);
   
    Paint.Rotate = Rotate;
    Paint.Mirror = MIRROR_NONE;
    
    if(Rotate == ROTATE_0 || Rotate == ROTATE_180) {
        Paint.Width = Width;
        Paint.Height = Height;
    } else {
        Paint.Width = Height;
        Paint.Height = Width;
    }
}

/******************************************************************************
function:	Select Image
parameter:
    image   :   Pointer to the image cache
******************************************************************************/
void Paint_SelectImage(UBYTE *image)
{
    Paint.Image = image;
}

/******************************************************************************
function:	Select Image Rotate
parameter:
    Rotate   :   0,90,180,270
******************************************************************************/
void Paint_SetRotate(UWORD Rotate)
{
    if(Rotate == ROTATE_0 || Rotate == ROTATE_90 || Rotate == ROTATE_180 || Rotate == ROTATE_270) {
        Debug("Set image Rotate %d", Rotate);
        Paint.Rotate = Rotate;
    } else {
        Debug("rotate = 0, 90, 180, 270");
    }
}

/******************************************************************************
function:	Select Image mirror
parameter:
    mirror   :       Not mirror,Horizontal mirror,Vertical mirror,Origin mirror
******************************************************************************/
void Paint_SetMirroring(UBYTE mirror)
{
    if(mirror == MIRROR_NONE || mirror == MIRROR_HORIZONTAL || 
        mirror == MIRROR_VERTICAL || mirror == MIRROR_ORIGIN) {
        Debug("mirror image x:%s, y:%s",(mirror & 0x01)? "mirror":"none", ((mirror >> 1) & 0x01)? "mirror":"none");
        Paint.Mirror = mirror;
    } else {
        Debug("mirror should be MIRROR_NONE, MIRROR_HORIZONTAL, \
        MIRROR_VERTICAL or MIRROR_ORIGIN");
    }    
}

/******************************************************************************
function:	Draw Pixels
parameter:
    Xpoint  :   At point X
    Ypoint  :   At point Y
    Color   :   Painted colors
******************************************************************************/
void Paint_SetPixel(UWORD Xpoint, UWORD Ypoint, UWORD Color)
{
    if(Xpoint > Paint.Width || Ypoint > Paint.Height){
        Debug("Exceeding display boundaries");
        return;
    }      
    UWORD X, Y;

    switch(Paint.Rotate) {
    case 0:
        X = Xpoint;
        Y = Ypoint;  
        break;
    case 90:
        X = Paint.WidthMemory - Ypoint - 1;
        Y = Xpoint;
        break;
    case 180:
        X = Paint.WidthMemory - Xpoint - 1;
        Y = Paint.HeightMemory - Ypoint - 1;
        break;
    case 270:
        X = Ypoint;
        Y = Paint.HeightMemory - Xpoint - 1;
        break;
		
    default:
        return;
    }
    
    switch(Paint.Mirror) {
    case MIRROR_NONE:
        break;
    case MIRROR_HORIZONTAL:
        X = Paint.WidthMemory - X - 1;
        break;
    case MIRROR_VERTICAL:
        Y = Paint.HeightMemory - Y - 1;
        break;
    case MIRROR_ORIGIN:
        X = Paint.WidthMemory - X - 1;
        Y = Paint.HeightMemory - Y - 1;
        break;
    default:
        return;
    }

    if(X > Paint.WidthMemory || Y > Paint.HeightMemory){
        Debug("Exceeding display boundaries");
        return;
    }
    
    UDOUBLE Addr = X / 8 + Y * Paint.WidthByte;
    UBYTE Rdata = Paint.Image[Addr];
    if(Color == BLACK)
        Paint.Image[Addr] = Rdata & ~(0x80 >> (X % 8));
    else
        Paint.Image[Addr] = Rdata | (0x80 >> (X % 8));
}

/******************************************************************************
function:	Clear the color of the picture
parameter:
    Color   :   Painted colors
******************************************************************************/
void Paint_Clear(UWORD Color)
{
    for (UWORD Y = 0; Y < Paint.HeightByte; Y++) {
        for (UWORD X = 0; X < Paint.WidthByte; X++ ) {//8 pixel =  1 byte
            UDOUBLE Addr = X + Y*Paint.WidthByte;
            Paint.Image[Addr] = Color;
        }
    }
}

/******************************************************************************
function:	Clear the color of a window
parameter:
    Xstart :   x starting point
    Ystart :   Y starting point
    Xend   :   x end point
    Yend   :   y end point
******************************************************************************/
void Paint_ClearWindows(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend, UWORD Color)
{
    UWORD X, Y;
    for (Y = Ystart; Y < Yend; Y++) {
        for (X = Xstart; X < Xend; X++) {//8 pixel =  1 byte
            Paint_SetPixel(X, Y, Color);
        }
    }
}

/******************************************************************************
function:	Draw Point(Xpoint, Ypoint) Fill the color
parameter:
    Xpoint		:   The Xpoint coordinate of the point
    Ypoint		:   The Ypoint coordinate of the point
    Color		:   Set color
    Dot_Pixel	:	point size
******************************************************************************/
void Paint_DrawPoint(UWORD Xpoint, UWORD Ypoint, UWORD Color,
                     DOT_PIXEL Dot_Pixel, DOT_STYLE DOT_STYLE)
{
    if (Xpoint > Paint.Width || Ypoint > Paint.Height) {
        Debug("Paint_DrawPoint Input exceeds the normal display range");
        return;
    }

    int16_t XDir_Num , YDir_Num;
    if (DOT_STYLE == DOT_FILL_AROUND) {
        for (XDir_Num = 0; XDir_Num < 2 * Dot_Pixel - 1; XDir_Num++) {
            for (YDir_Num = 0; YDir_Num < 2 * Dot_Pixel - 1; YDir_Num++) {
                if(Xpoint + XDir_Num - Dot_Pixel < 0 || Ypoint + YDir_Num - Dot_Pixel < 0)
                    break;
                Paint_SetPixel(Xpoint + XDir_Num - Dot_Pixel, Ypoint + YDir_Num - Dot_Pixel, Color);
            }
        }
    } else {
        for (XDir_Num = 0; XDir_Num <  Dot_Pixel; XDir_Num++) {
            for (YDir_Num = 0; YDir_Num <  Dot_Pixel; YDir_Num++) {
                Paint_SetPixel(Xpoint + XDir_Num - 1, Ypoint + YDir_Num - 1, Color);
            }
        }
    }
}

/******************************************************************************
function:	Draw a line of arbitrary slope
parameter:
    Xstart Starting Xpoint point coordinates
    Ystart Starting Xpoint point coordinates
    Xend   End point Xpoint coordinate
    Yend   End point Ypoint coordinate
    Color  The color of the line segment
******************************************************************************/
void Paint_DrawLine(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend,
                    UWORD Color, LINE_STYLE Line_Style, DOT_PIXEL Dot_Pixel)
{
    if (Xstart > Paint.Width || Ystart > Paint.Height ||
        Xend > Paint.Width || Yend > Paint.Height) {
        Debug("Paint_DrawLine Input exceeds the normal display range");
        return;
    }

    UWORD Xpoint = Xstart;
    UWORD Ypoint = Ystart;
    int dx = (int)Xend - (int)Xstart >= 0 ? Xend - Xstart : Xstart - Xend;
    int dy = (int)Yend - (int)Ystart <= 0 ? Yend - Ystart : Ystart - Yend;

    // Increment direction, 1 is positive, -1 is counter;
    int XAddway = Xstart < Xend ? 1 : -1;
    int YAddway = Ystart < Yend ? 1 : -1;

    //Cumulative error
    int Esp = dx + dy;
    char Dotted_Len = 0;

    for (;;) {
        Dotted_Len++;
        //Painted dotted line, 2 point is really virtual
        if (Line_Style == LINE_STYLE_DOTTED && Dotted_Len % 3 == 0) {
            Paint_DrawPoint(Xpoint, Ypoint, IMAGE_BACKGROUND, Dot_Pixel, DOT_STYLE_DFT);
            Dotted_Len = 0;
        } else {
            Paint_DrawPoint(Xpoint, Ypoint, Color, Dot_Pixel, DOT_STYLE_DFT);
        }
        if (2 * Esp >= dy) {
            if (Xpoint == Xend)
                break;
            Esp += dy;
            Xpoint += XAddway;
        }
        if (2 * Esp <= dx) {
            if (Ypoint == Yend)
                break;
            Esp += dx;
            Ypoint += YAddway;
        }
    }
}

/******************************************************************************
function:	Draw a rectangle
parameter:
    Xstart Rectangular  Starting Xpoint point coordinates
    Ystart Rectangular  Starting Xpoint point coordinates
    Xend   Rectangular  End point Xpoint coordinate
    Yend   Rectangular  End point Ypoint coordinate
    Color  The color of the Rectangular segment
    Filled : Whether it is filled--- 1 solid 0empty
******************************************************************************/
void Paint_DrawRectangle(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend,
                         UWORD Color, DRAW_FILL Filled, DOT_PIXEL Dot_Pixel)
{
    if (Xstart > Paint.Width || Ystart > Paint.Height ||
        Xend > Paint.Width || Yend > Paint.Height) {
        Debug("Input exceeds the normal display range");
        return;
    }

    if (Filled ) {
        UWORD Ypoint;
        for(Ypoint = Ystart; Ypoint <= Yend; Ypoint++) {
            Paint_DrawLine(Xstart, Ypoint, Xend, Ypoint, Color , LINE_STYLE_SOLID, Dot_Pixel);
        }
    } else {
        Paint_DrawLine(Xstart, Ystart, Xend, Ystart, Color , LINE_STYLE_SOLID, Dot_Pixel);
        Paint_DrawLine(Xstart, Ystart, Xstart, Yend, Color , LINE_STYLE_SOLID, Dot_Pixel);
        Paint_DrawLine(Xend, Yend, Xend, Ystart, Color , LINE_STYLE_SOLID, Dot_Pixel);
        Paint_DrawLine(Xend, Yend, Xstart, Yend, Color , LINE_STYLE_SOLID, Dot_Pixel);
    }
}

/******************************************************************************
function:	Use the 8-point method to draw a circle of the
            specified size at the specified position->
parameter:
    X_Center  Center X coordinate
    Y_Center  Center Y coordinate
    Radius    circle Radius
    Color     The color of the circle segment
    Filled    : Whether it is filled: 1 filling 0Do not
******************************************************************************/
void Paint_DrawCircle(UWORD X_Center, UWORD Y_Center, UWORD Radius,
                      UWORD Color, DRAW_FILL  Draw_Fill , DOT_PIXEL Dot_Pixel)
{
    if (X_Center > Paint.Width || Y_Center >= Paint.Height) {
        Debug("Paint_DrawCircle Input exceeds the normal display range");
        return;
    }

    //Draw a circle from(0, R) as a starting point
    int16_t XCurrent, YCurrent;
    XCurrent = 0;
    YCurrent = Radius;

    //Cumulative error,judge the next point of the logo
    int16_t Esp = 3 - (Radius << 1 );

    int16_t sCountY;
    if (Draw_Fill == DRAW_FILL_FULL) {
        while (XCurrent <= YCurrent ) { //Realistic circles
            for (sCountY = XCurrent; sCountY <= YCurrent; sCountY ++ ) {
                Paint_DrawPoint(X_Center + XCurrent, Y_Center + sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//1
                Paint_DrawPoint(X_Center - XCurrent, Y_Center + sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//2
                Paint_DrawPoint(X_Center - sCountY, Y_Center + XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//3
                Paint_DrawPoint(X_Center - sCountY, Y_Center - XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//4
                Paint_DrawPoint(X_Center - XCurrent, Y_Center - sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//5
                Paint_DrawPoint(X_Center + XCurrent, Y_Center - sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//6
                Paint_DrawPoint(X_Center + sCountY, Y_Center - XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//7
                Paint_DrawPoint(X_Center + sCountY, Y_Center + XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);
            }
            if (Esp < 0 )
                Esp += 4 * XCurrent + 6;
            else {
                Esp += 10 + 4 * (XCurrent - YCurrent );
                YCurrent --;
            }
            XCurrent ++;
        }
    } else { //Draw a hollow circle
        while (XCurrent <= YCurrent ) {
            Paint_DrawPoint(X_Center + XCurrent, Y_Center + YCurrent, Color, Dot_Pixel, DOT_STYLE_DFT);//1
            Paint_DrawPoint(X_Center - XCurrent, Y_Center + YCurrent, Color, Dot_Pixel, DOT_STYLE_DFT);//2
            Paint_DrawPoint(X_Center - YCurrent, Y_Center + XCurrent, Color, Dot_Pixel, DOT_STYLE_DFT);//3
            Paint_DrawPoint(X_Center - YCurrent, Y_Center - XCurrent, Color, Dot_Pixel, DOT_STYLE_DFT);//4
            Paint_DrawPoint(X_Center - XCurrent, Y_Center - YCurrent, Color, Dot_Pixel, DOT_STYLE_DFT);//5
            Paint_DrawPoint(X_Center + XCurrent, Y_Center - YCurrent, Color, Dot_Pixel, DOT_STYLE_DFT);//6
            Paint_DrawPoint(X_Center + YCurrent, Y_Center - XCurrent, Color, Dot_Pixel, DOT_STYLE_DFT);//7
            Paint_DrawPoint(X_Center + YCurrent, Y_Center + XCurrent, Color, Dot_Pixel, DOT_STYLE_DFT);//0

            if (Esp < 0 )
                Esp += 4 * XCurrent + 6;
            else {
                Esp += 10 + 4 * (XCurrent - YCurrent );
                YCurrent --;
            }
            XCurrent ++;
        }
    }
}

/******************************************************************************
function:	Show English characters
parameter:
    Xpoint           X coordinate
    Ypoint           Y coordinate
    Acsii_Char       To display the English characters
    Font             A structure pointer that displays a character size
    Color_Background : Select the background color of the English character
    Color_Foreground : Select the foreground color of the English character
******************************************************************************/
void Paint_DrawChar(UWORD Xpoint, UWORD Ypoint, const char Acsii_Char,
                    sFONT* Font, UWORD Color_Background, UWORD Color_Foreground)
{
    UWORD Page, Column;

    if (Xpoint > Paint.Width || Ypoint > Paint.Height) {
        Debug("Paint_DrawChar Input exceeds the normal display range");
        return;
    }

    uint32_t Char_Offset = (Acsii_Char - ' ') * Font->Height * (Font->Width / 8 + (Font->Width % 8 ? 1 : 0));
    const unsigned char *ptr = &Font->table[Char_Offset];

    for (Page = 0; Page < Font->Height; Page ++ ) {
        for (Column = 0; Column < Font->Width; Column ++ ) {

            //To determine whether the font background color and screen background color is consistent
            if (FONT_BACKGROUND == Color_Background) { //this process is to speed up the scan
                if (*ptr & (0x80 >> (Column % 8)))
                    Paint_SetPixel(Xpoint + Column, Ypoint + Page, Color_Foreground);
            } else {
                if (*ptr & (0x80 >> (Column % 8))) {
                    Paint_SetPixel(Xpoint + Column, Ypoint + Page, Color_Foreground);
                } else {
                    Paint_SetPixel(Xpoint + Column, Ypoint + Page, Color_Background);
                }
            }
            //One pixel is 8 bits
            if (Column % 8 == 7)
                ptr++;
        }// Write a line
        if (Font->Width % 8 != 0)
            ptr++;
    }// Write all
}

/******************************************************************************
function:	Display the string
parameter:
    Xstart           X coordinate
    Ystart           Y coordinate
    pString          The first address of the English string to be displayed
    Font             A structure pointer that displays a character size
    Color_Background : Select the background color of the English character
    Color_Foreground : Select the foreground color of the English character
******************************************************************************/
void Paint_DrawString(UWORD Xstart, UWORD Ystart, const char * pString,
                         sFONT* Font, UWORD Color_Background, UWORD Color_Foreground )
{
    UWORD Xpoint = Xstart;
    UWORD Ypoint = Ystart;

    if (Xstart > Paint.Width || Ystart > Paint.Height) {
        Debug("Paint_DrawString_EN Input exceeds the normal display range");
        return;
    }

    while (* pString != '\0') {
        //if X direction filled , reposition to(Xstart,Ypoint),Ypoint is Y direction plus the Height of the character
        if ((Xpoint + Font->Width ) > Paint.Width ) {
            Xpoint = Xstart;
            Ypoint += Font->Height;
        }

        // If the Y direction is full, reposition to(Xstart, Ystart)
        if ((Ypoint  + Font->Height ) > Paint.Height ) {
            Xpoint = Xstart;
            Ypoint = Ystart;
        }
        Paint_DrawChar(Xpoint, Ypoint, * pString, Font, Color_Background, Color_Foreground);

        //The next character of the address
        pString ++;

        //The next word of the abscissa increases the font of the broadband
        Xpoint += Font->Width;
    }
}


/******************************************************************************
function:	Display monochrome bitmap
parameter:
    image_buffer A picture data converted to a bitmap
info:
    Use a computer to convert the image into a corresponding array,
    and then embed the array directly into Imagedata.cpp as a .c file.
******************************************************************************/
void Paint_DrawBitMap(const unsigned char* image_buffer)
{
    DMA_memcpy(Paint.Image, (void *)image_buffer, (Paint.WidthByte*Paint.HeightByte -1));
}
