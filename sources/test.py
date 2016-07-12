import os
import math
import subprocess
import matplotlib.pyplot as plt

path = os.path.dirname(os.path.realpath(__file__))
execpath = path + '/../build/EmailRecognition'
cardpath = path + '/../cards/dataset/'

cardpath0 = path + '/../cards/300/'
data = [
    ['card-01.jpg', [[804, 474], [1806, 492], [794, 1069], [1799, 1081]]],
    ['card-02.jpg', [[285, 467], [291, 1057], [1339, 455], [1347, 1041]]],
    ['card-03.jpg', [[569, 511], [1519, 936], [1274, 1482], [319, 1066]]],
    ['card-06.jpg', [[614, 159], [1263, 155], [1271, 1216], [627, 1225]]],
    ['card-07.jpg', [[1004, 501], [1943, 962], [1679, 1496], [744, 1039]]],
    ['card-09.jpg', [[75, 267], [1179, 209], [1206, 822], [102, 871]]],
    ['card-12.jpg', [[234, 334], [1287, 386], [1261, 975], [208, 922]]],
    ['card-13.jpg', [[398, 261], [1374, 542], [1235, 1042], [261, 761]]],
]

def parse(raw):
    return map(lambda x: map(float, x.split(' ')), raw.strip().split('\n'))

def ordered(rect):
    avg_x = sum(map(lambda a: a[0], rect)) / 4.0
    avg_y = sum(map(lambda a: a[1], rect)) / 4.0
    return sorted(rect, key=lambda a: math.atan2(a[1] - avg_y, a[0] - avg_x))

def dist((a, b)):
    return ((a[0]-b[0])**2 + (a[1]-b[1])**2)

def grade(out, ans):
    return sum(map(dist, zip(ans, out)))

def test_card():
    count_all = 0
    count_good = 0
    data = []
    for f in os.listdir(cardpath):
        if os.path.isfile(cardpath + f):
            print f,
            out = subprocess.check_output([execpath, '-cut', cardpath + f])
            ans = open(cardpath + f + '.mark/bounds.quad').read()
            # print ordered(parse(out))
            # print ordered(parse(ans))
            res = grade(ordered(parse(out)), ordered(parse(ans)))
            data.append(res)
            count_all += 1
            if res < 1e6:
                count_good += 1
                print '+', res
            else:
                print '-', res
    print count_good / float(count_all)
    return data

def test_email():
    files = [cardpath + f for f in os.listdir(cardpath) if os.path.isfile(cardpath + f)]
    # print files
    files = files[:20]
    out = subprocess.check_output([execpath, '-text'] + files)
    print out
    # for f in files:
        # print f,
        # ans = open(f + '.mark/bounds.quad').read()
        # print ordered(parse(out))
        # print ordered(parse(ans))
        # res = grade(/ordered(parse(out)), ordered(parse(ans)))
        # print ans

# data = test_card()
# plt.scatter(range(len(data)), data)
# plt.savefig("plot.png", dpi=300)

test_email()