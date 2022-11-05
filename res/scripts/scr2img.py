import sys
import os
import numpy as np
from PIL import Image


def usage_print():
    print("Usage: python " + sys.argv[0] + " black_epd_file grey_epd_file red_epd_file output_img_file")
    print("Combine epd buffers into an image file")


if __name__ == '__main__':
    blackColor = (0, 0, 0)
    redColor = (237, 28, 36)
    greyColor = (195, 195, 195)
    pinkColor = (239, 136, 190)
    whiteColor = (255, 255, 255)

    if len(sys.argv) == 1:
        usage_print()
        exit(0)

    if len(sys.argv) != 5:
        print("Wrong arguments number!")
        usage_print()
        exit(1)

    filePathBlack = sys.argv[1]
    filePathGrey = sys.argv[2]
    filePathRed = sys.argv[3]
    filePathOutput = sys.argv[4]

    outImgArray = np.full([200, 200, 3], whiteColor, dtype=np.uint8)

    cc = 0
    rc = 0
    with open(filePathGrey, "rb") as f:
        while byte := f.read(1):
            byte = int.from_bytes(byte, byteorder='big')
            if not (byte & 0b10000000):
                outImgArray[rc, 8 * cc + 0] = greyColor
            if not (byte & 0b01000000):
                outImgArray[rc, 8 * cc + 1] = greyColor
            if not (byte & 0b00100000):
                outImgArray[rc, 8 * cc + 2] = greyColor
            if not (byte & 0b00010000):
                outImgArray[rc, 8 * cc + 3] = greyColor
            if not (byte & 0b00001000):
                outImgArray[rc, 8 * cc + 4] = greyColor
            if not (byte & 0b00000100):
                outImgArray[rc, 8 * cc + 5] = greyColor
            if not (byte & 0b00000010):
                outImgArray[rc, 8 * cc + 6] = greyColor
            if not (byte & 0b00000001):
                outImgArray[rc, 8 * cc + 7] = greyColor

            cc += 1
            if cc >= 25:
                cc = 0
                rc += 1

    cc = 0
    rc = 0
    with open(filePathBlack, "rb") as f:
        while byte := f.read(1):
            byte = int.from_bytes(byte, byteorder='big')
            if not (byte & 0b10000000):
                outImgArray[rc, 8 * cc + 0] = blackColor
            if not (byte & 0b01000000):
                outImgArray[rc, 8 * cc + 1] = blackColor
            if not (byte & 0b00100000):
                outImgArray[rc, 8 * cc + 2] = blackColor
            if not (byte & 0b00010000):
                outImgArray[rc, 8 * cc + 3] = blackColor
            if not (byte & 0b00001000):
                outImgArray[rc, 8 * cc + 4] = blackColor
            if not (byte & 0b00000100):
                outImgArray[rc, 8 * cc + 5] = blackColor
            if not (byte & 0b00000010):
                outImgArray[rc, 8 * cc + 6] = blackColor
            if not (byte & 0b00000001):
                outImgArray[rc, 8 * cc + 7] = blackColor

            cc += 1
            if cc >= 25:
                cc = 0
                rc += 1

    cc = 0
    rc = 0
    with open(filePathRed, "rb") as f:
        while byte := f.read(1):
            byte = int.from_bytes(byte, byteorder='big')
            if not (byte & 0b10000000):
                if (outImgArray[rc, 8 * cc + 0] == blackColor).all(-1):
                    outImgArray[rc, 8 * cc + 0] = redColor
                else:
                    outImgArray[rc, 8 * cc + 0] = pinkColor
            if not (byte & 0b01000000):
                if (outImgArray[rc, 8 * cc + 1] == blackColor).all(-1):
                    outImgArray[rc, 8 * cc + 1] = redColor
                else:
                    outImgArray[rc, 8 * cc + 1] = pinkColor
            if not (byte & 0b00100000):
                if (outImgArray[rc, 8 * cc + 2] == blackColor).all(-1):
                    outImgArray[rc, 8 * cc + 2] = redColor
                else:
                    outImgArray[rc, 8 * cc + 2] = pinkColor
            if not (byte & 0b00010000):
                if (outImgArray[rc, 8 * cc + 3] == blackColor).all(-1):
                    outImgArray[rc, 8 * cc + 3] = redColor
                else:
                    outImgArray[rc, 8 * cc + 3] = pinkColor
            if not (byte & 0b00001000):
                if (outImgArray[rc, 8 * cc + 4] == blackColor).all(-1):
                    outImgArray[rc, 8 * cc + 4] = redColor
                else:
                    outImgArray[rc, 8 * cc + 4] = pinkColor
            if not (byte & 0b00000100):
                if (outImgArray[rc, 8 * cc + 5] == blackColor).all(-1):
                    outImgArray[rc, 8 * cc + 5] = redColor
                else:
                    outImgArray[rc, 8 * cc + 5] = pinkColor
            if not (byte & 0b00000010):
                if (outImgArray[rc, 8 * cc + 6] == blackColor).all(-1):
                    outImgArray[rc, 8 * cc + 6] = redColor
                else:
                    outImgArray[rc, 8 * cc + 6] = pinkColor
            if not (byte & 0b00000001):
                if (outImgArray[rc, 8 * cc + 7] == blackColor).all(-1):
                    outImgArray[rc, 8 * cc + 7] = redColor
                else:
                    outImgArray[rc, 8 * cc + 7] = pinkColor

            cc += 1
            if cc >= 25:
                cc = 0
                rc += 1

    Image.fromarray(outImgArray, mode="RGB").save(filePathOutput)
