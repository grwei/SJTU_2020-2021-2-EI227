import cv2

def getHex(img, row, col):
    num = 0
    for i in range(0, 8):
        num += (1-img[row+i][col]) * 2 ** (7-i)
    return '0x' + str('{0:02X}'.format(num))


absolutePath = input('Absolute path:')
iformat = input('文件格式：')
number = int(input('图片数量(注意图片名称从0开始)：'))

for i in range(0, number):
    img = cv2.imread(absolutePath + '\\' + str(i) + '.' + iformat)
    imgg = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    small = cv2.resize(imgg, (128, 64))
    ret, binary = cv2.threshold(small, 100, 1, cv2.THRESH_BINARY)
    binary = cv2.flip(binary, 0)
    file = open(absolutePath + '\\' + str(i) + '.txt', 'w')

    for i in range(7, -1, -1):
        k = 0
        for j in range(0, 128):
            k += 1
            file.write(getHex(binary, i*8, j) + ',\t')
            if k == 16:
                k = 0
                file.write('\n')

    file.close()


