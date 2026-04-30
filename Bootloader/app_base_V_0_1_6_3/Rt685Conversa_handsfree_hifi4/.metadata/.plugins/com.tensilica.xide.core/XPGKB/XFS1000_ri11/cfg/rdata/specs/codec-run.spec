
what          Codec Run`1
name_ex       ^.*xtensa-system.* ([^ ]+)$

colnames_s    Frame`This MCPS`Average MCPS`Peak MCPS
start_ex      ^Output Channel Map:.*

values_ex     ^\[\d+\|(\d+)\] .*MCPS: ([\d\.]+) Average: ([\d\.]+) Peak: ([\d\.]+).*

#
#Q:\xxx\install\tools\RI-2019.2-win32\XtensaTools\bin\xt-run --xtensa-core=hifi3_ao_5 --xtensa-system=Q:\xxx\install\builds\RI-2019.2-win32\hifi3_ao_5\config --xtensa-params= --console --turbo --summary Q:\p4\pmac_xhw_nt\tws\t81\testxa_aacplus_v2_dec\bin\hifi3_ao_5\Release\testxa_aacplus_v2_dec
#
#-w16 -ifile:thetest.adts -ofile:thetest.wav 
#
#
#aacPlus v2 (2-ch) Decoder version 3.7
#Tensilica, Inc. http://www.tensilica.com
#
#
#API structure size: 188 bytes
#MEMTABS size: 144 bytes
#Persistent buffer size: 29560 bytes
#Scratch buffer size: 22720 bytes
#Input buffer size: 1824 bytes
#Output buffer size: 16384 bytes
#Bitstream format:   ADTS
#Format:             aacPlus v2
#Output Sample rate: 48000 Hz
#AAC Sample rate:    24000 Hz
#Input config:       Parametric Stereo
#Output Channels:    2
#Output Channel Map: ffffff20
#[1|0] 0:00.042 MCPS: 15.73 Average: 15.73 Peak: 15.73 @ [1]
#[2|256] 0:00.085 MCPS: 16.45 Average: 16.09 Peak: 16.45 @ [2]
