from subprocess import Popen, PIPE
import sys
import re

colstore = "../colstore"
attribute_id = 0;
return_attribute_id = 1;
start = "start"
end = "end"

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
        ['../select3', colstore, '%d' % attribute_id, return_attribute_id ,start, end , '%d' % sz],
        stdout=PIPE)
    s = p.stdout.read().strip()
    print '>%s' % s
    perf_y.append( float(re.findall(r'\d+', s)[0]) * num_of_rec / 1000)
    perf_x.append( sz)

import matplotlib
from pylab import *
figure(1)
loglog(perf_x, perf_y)
xlabel('page size')
ylabel('record/s')
savefig('select3.png')