#!/usr/bin/python

import sys
from collections import namedtuple
from datetime import datetime
import numpy as np
import matplotlib.pyplot as plt


sample = namedtuple('sample', ('ts', 'addr', 'length', 'repeat', 'rtt', 'stdev'))

unpackers = (lambda d: datetime.fromtimestamp(int(d)), str, int, int, float, float)

def unpack(line):
    s = line.split()
    return sample(*(unpackers[i](s[i]) for i in xrange(len(s))))

if len(sys.argv) < 2:
    print 'usage: {0} rtt_file'.format(sys.argv[0])

with open(sys.argv[1]) as f:
    data = map(unpack, f)

def plot(data):
    length = np.array([s.length for s in data])
    rtt = np.array([s.rtt for s in data])
    stdev = np.array([s.stdev for s in data])


    #print range(len(data)), rtt, stdev
    plt.errorbar(xrange(len(data)), rtt, stdev, linestyle='None', marker='^')

    ax = plt.gca()
    ax.set_xticks(xrange(len(data)))
    ax.set_xticklabels(map(str, length))
    plt.ylim(ymin=0.0)

    plt.show()

for k in xrange(0, len(data), 10):
    plot(data[k:k+10])

