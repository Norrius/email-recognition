import math
import subprocess

path = '../cards/300/'
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


# for line in open(path + 'points'):
#     coords = map(lambda x: x.split(','), line.split('#')[0].split(';'))
#     for e in coords:
#         print map(lambda x: int(x), e)

def dist(a, b):
    return ((a[0]-b[0])**2 + (a[1]-b[1])**2)**0.5

for item in data:
    out = subprocess.check_output(['../build/EmailRecognition', path + item[0], '-s'])
    coords_out = sorted(eval(out), key=lambda l: math.atan2(l[1], l[0]))
    coords_ans = sorted(item[1], key=lambda l: math.atan2(l[1], l[0]))
    diff = 0
    for i in range(4):
        diff += dist(coords_out[i], coords_ans[i])
    print item[0], diff
    # print coords_ans
    # print map(lambda l: math.atan2(l[1], l[0]), coords_ans)
    # print coords_out
    # print map(lambda l: math.atan2(l[1], l[0]), coords_out)
