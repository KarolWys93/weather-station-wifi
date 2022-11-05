import sys
import os
import numpy as np
from PIL import Image

if __name__ == '__main__':
    blackColor = (0, 0, 0)
    redColor = (237, 28, 36)
    greyColor = (195, 195, 195)
    pinkColor = (239, 136, 190)

    if len(sys.argv) == 1:
        print("Usage: python "+sys.argv[0]+" img_file")
        print("Split a color image into three monochrome images corresponding to the colors supported by the e-paper display:")
        print("black, grey and red.")
        print("Supported image colors:")
        print("0x000000 : black")
        print("0xED1C24 : red")
        print("0xC3C3C3 : grey")
        print("0xEF88BE : pink")
        print("0xFFFFFF : white")
        exit(0)

    if len(sys.argv) != 2:
        print("Wrong arguments number!")
        exit(1)

    filePath = sys.argv[1]
    filePathPattern = os.path.splitext(filePath)[0]

    img = Image.open(filePath).convert("RGB")

    imgArray = np.array(img)

    mask = ~((imgArray == blackColor).all(-1) | (imgArray == redColor).all(-1))
    Image.fromarray(mask).convert("1").save(filePathPattern+"_b.bmp")

    mask = ~((imgArray == greyColor).all(-1))
    Image.fromarray(mask).convert("1").save(filePathPattern+"_g.bmp")

    mask = ~((imgArray == pinkColor).all(-1) | (imgArray == redColor).all(-1))
    Image.fromarray(mask).convert("1").save(filePathPattern+"_r.bmp")
