
what          ISS Cycle Summary`1
name_ex       ^.*xtensa-system.* ([^ ]+)$

start_ex      CPI\s+CPI.*Cycle

rownames_ex   ^([\w\s]+)\s\s+\d+\s.\s*\d.*

colnames_s    Cycles
values_ex     ^[\w\s]+\s\s+(\d+)\s.\s*[\d\.]+\s+[\d\.]+..\s*[\d\.]+\s+.*

#colnames_s    Cycles`CPI`Summed CPI`%Cycle`Summed %Cycle
#values_ex     ^[\w\s]+\s\s+(\d+)\s.\s*([\d\.]+)\s+([\d\.]+)..\s*([\d\.]+)\s+([\d\.]+).*

#                                        CPI      CPI   |% Cycle  % Cycle
#Committed instructions        411345 ( 1.0000   1.0000 |  69.46    69.46 )
#Taken branches                 49249 ( 0.1197   1.1197 |   8.32    77.78 )
#Pipeline interlocks           131508 ( 0.3197   1.4394 |  22.21    99.99 )
#Exceptions                         5 ( 0.0000   1.4394 |   0.00    99.99 )
#Sync replays                      48 ( 0.0001   1.4396 |   0.01   100.00 )
#Special instructions              18 ( 0.0000   1.4396 |   0.00   100.00 )
#Reset                              5 ( 0.0000   1.4396 |   0.00   100.00 )
