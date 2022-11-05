import sys
import os
import numpy as np
from PIL import Image

if __name__ == '__main__':

    if len(sys.argv) == 1:
        print("Usage: python "+sys.argv[0]+" img_file")
        print("Convert image to epd file. The output file is saved to the location of the input image file.")
        exit(0)

    if len(sys.argv) != 2:
        print("Wrong arguments number!")
        exit(1)

    filePath = sys.argv[1]
    filePathPattern = os.path.splitext(filePath)[0]

    img = Image.open(filePath).convert("1")
    if img.size != (200, 200):
        print("wrong image size")
        exit(1)
    output = []
    imgArray = np.array(img)

    xRange = range(0, 25)
    yRange = range(0, 200)
    for y in yRange:
        for x in xRange:

            segment = 128*imgArray[y, 8*x]\
                      + 64*imgArray[y, 8*x+1]\
                      + 32*imgArray[y, 8*x+2]\
                      + 16*imgArray[y, 8*x+3]\
                      + 8*imgArray[y, 8*x+4]\
                      + 4*imgArray[y, 8*x+5]\
                      + 2*imgArray[y, 8*x+6]\
                      + imgArray[y, 8*x+7]
            output.append(segment)

    newfile = open(filePathPattern+".epd", 'wb')
    newFileByteArray = bytearray(output)
    newfile.write(newFileByteArray)
