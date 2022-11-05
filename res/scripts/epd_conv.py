import sys
import os
import numpy as np
from PIL import Image


def usage_print():
    print("Usage: python " + sys.argv[0] + " [options] img_file")
    print("Convert image to epd file. The output file is saved to the location of the input image file.")
    print("Options: ")
    print("-t    save file as a text")


if __name__ == '__main__':
    saveAsText = False
    filePath = ""

    if len(sys.argv) == 1:
        usage_print()
        exit(0)

    if len(sys.argv) > 3:
        print("Wrong arguments number!")
        usage_print()
        exit(1)

    if len(sys.argv) == 3:
        if sys.argv[1] != "-t":
            print("Wrong parameters!")
            usage_print()
            exit(1)
        else:
            saveAsText = True
            filePath = sys.argv[2]
    else:
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
            segment = 128 * imgArray[y, 8 * x] \
                      + 64 * imgArray[y, 8 * x + 1] \
                      + 32 * imgArray[y, 8 * x + 2] \
                      + 16 * imgArray[y, 8 * x + 3] \
                      + 8 * imgArray[y, 8 * x + 4] \
                      + 4 * imgArray[y, 8 * x + 5] \
                      + 2 * imgArray[y, 8 * x + 6] \
                      + imgArray[y, 8 * x + 7]
            output.append(segment)

    newFileByteArray = bytearray(output)
    if not saveAsText:
        newfile = open(filePathPattern + ".epd", 'wb')
        newfile.write(newFileByteArray)
    else:
        columnCounter = 0
        newfile = open(filePathPattern + ".txt", 'w')
        for i in newFileByteArray:
            if columnCounter == 0:
                print("\t\t", end='', file=newfile)
            print("0X{:02X},".format(i), end='', file=newfile)
            columnCounter += 1
            if columnCounter == 16:
                print("\n", end='', file=newfile)
                columnCounter = 0
