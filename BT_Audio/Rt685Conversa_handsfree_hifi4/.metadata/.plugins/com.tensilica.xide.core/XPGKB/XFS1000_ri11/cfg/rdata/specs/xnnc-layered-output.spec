
what          XNNC ISS Layered Output`1`Count
name_ex       ^.*xtensa-system.* ([^ ]+).*$

start_ex      ^RUNNING GRAPH.*

rownames_ex   ^\d+ \s*\d+ \s*\d+ \s*\d+ \s*\d+ \s*[\.\d]+ \s*[\.\d]+ \s*\d+ \s*\d+ \s*(.*)$
colnames_s    Cycles`XI Kernel Cycles`Edge Ext Cycles`DSP Idle Wait Cycles`MACs\Cycle`MAC%`MACs`DMA Queue Size
values_ex     ^\d+ \s*(\d+) \s*(\d+) \s*(\d+) \s*(\d+) \s*([\.\d]+) \s*([\.\d]+) \s+(\d+) \s+(\d+)\s+.*$

#end_ex        ^TOTAL\s+.*

#RUNNING GRAPH...
#                         XI     Edge       DSP     MACs                      DMA
#           Total     Kernel      Ext Idle WAIT      per    MAC             Queue
#    #     Cycles     Cycles   Cycles    Cycles    Cycle      %        MACs  Size Layer Name
#----- ---------- ---------- -------- --------- -------- ------ ----------- ----- -----------------------------
#    1    1033589    1003026    14641      4934   114.18  44.60   118013952     5 Conv__432_whd
#    2     184175     170251        0      3914     0.00   0.00           0     2 transpose_Conv__432_res_whd_dwh
#    3      89878      49590     9012     25498     0.00   0.00           0     2 resnet_v1_50_pool1_MaxPool__1_dwh
#    4     160422     152545        0      2284    80.07  31.28    12845056     3 Conv__433_dwh
