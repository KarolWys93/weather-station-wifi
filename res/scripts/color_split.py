import sys
import os
import numpy as np
from PIL import Image

if __name__ == '__main__':
    blackColor = (0, 0, 0)
    redColor = (255, 0, 0)
    greyColor = (195, 195, 195)
    pinkColor = (239, 136, 190)

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
