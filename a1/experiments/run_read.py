from subprocess import Popen, PIPE
import sys
import re
import time

filename = "test.csv"
page_file = "pagefile"
num_of_rec = 10000
page_size = (
    1002,
    2000,
    4000,
    8000,
    16000,
    32000,
    64000,
    128000,
    256000,
    512000
)

perf_x = []
perf_y = []
for sz in page_size:
    p = Popen(
        ['../write_fixed_len_pages', filename, page_file, '%d' % sz],
        stdout=PIPE)
    time.sleep(1)
    p = Popen(
        ['../read_fixed_len_pages', page_file, '%d' % sz],
        stdout=PIPE)
    s = p.stdout.read().strip()
    print '>%s' % s
    print '%d' % sz
    perf_y.append( num_of_rec/ (float(re.findall(r'\d+', s)[0]) * 1000))
    perf_x.append( sz)

import matplotlib
from pylab import *
figure(1)
loglog(perf_x, perf_y)
xlabel('page size')
ylabel('record/s')
savefig('read.png')