#!/usr/bin/python2
# coding=utf-8

# 大部分 linux 默认装有 python，用 python 写比较通用

import sys
import time
import os.path
import datetime
import decimal

def Entry():


    PrintHead()

    logFormat="/var/log/aps-agent/stat_sec-%s.log"

    seekEnd = 1
    curDate = datetime.datetime.now().strftime('%Y%m%d')

    while True:

        logFile = logFormat % (curDate)

        try:
            with open(logFile,'r') as f:

                if seekEnd is 1:
                    f.seek(0,2)

                readyToBreak = 0

                while True:

                    line = f.readline()
                    if line:
                        PrintLine(line)
                        continue

                    try:
                        # check current open file deleted
                        st_actual = os.stat(logFile)
                        st_cur = os.fstat(f.fileno())
                        if (st_actual.st_ino!=st_cur.st_ino) and (readyToBreak is 0):
                            # go another round, make sure all tailing contents are read
                            readyToBreak = 1
                            continue
                    except:
                        True

                    # check new file created
                    newDate = datetime.datetime.now().strftime('%Y%m%d')
                    newFile = logFormat%(newDate)
                    if (newDate!=curDate) and (readyToBreak is 0):
                        # go another round, make sure all tailing contents are read
                        readyToBreak = os.path.isfile(newFile)
                        if readyToBreak:
                            curDate = newDate
                        continue

                    if readyToBreak:
                        seekEnd = 0
                        break


                    time.sleep(0.1)

        except Exception,e:

            print e

            curDate = datetime.datetime.now().strftime('%Y%m%d')

            time.sleep(1)


sysmodSizeMax = 52;
sysmod1Size   = 30;
sysmod2Size   = 30;

def PrintLine(line):

    global sysmodSizeMax 
    global sysmod1Size 
    global sysmod2Size 

    key_val = line.split('|')
    keys = key_val[0].split('\t')
    vals = key_val[1].split('\t')

    time    = keys[0];
    sys1    = keys[1];
    mod1    = keys[2];
    sys2    = keys[3];
    mod2    = keys[4];
    ip      = keys[5];
    port    = keys[6];
    cnt     = vals[0];
    suc_cnt = vals[1];
    
    count         = decimal.Decimal(vals[0])
    successCount  = decimal.Decimal(vals[1])
    timeUs        = decimal.Decimal(vals[2])
    successTimeUs = decimal.Decimal(vals[3])

    successPercent    = float(successCount/count*decimal.Decimal("100"))
    meanTimeMs        = float(timeUs/count/decimal.Decimal('1000'))
    if successCount != 0:
        successMeanTimeMs = float(successTimeUs/successCount/decimal.Decimal('1000'))
    else:
        successMeanTimeMs = float(0)
    if (count-successCount)!= 0:
        failMeanTimeMs    = float((timeUs-successTimeUs)/(count-successCount)/decimal.Decimal('1000'))
    else:
        failMeanTimeMs = float(0)

    adjustHead = 0
    
    modsys1 = sys1+":"+mod1
    modsys2 = sys2+":"+mod2
    if (len(modsys1) > sysmod1Size) and (len(modsys1) <= sysmodSizeMax):
        sysmod1Size = len(modsys1)
        adjustHead = 1
    if (len(modsys2) > sysmod2Size) and (len(modsys2) <= sysmodSizeMax):
        sysmod2Size = len(modsys2)
        adjustHead = 1

    # if adjustHead == 1:
    #     sys.stdout.write("\n")
    #     PrintHead()
    
    sys.stdout.write( (("%20s | %"+str(sysmod1Size)+"s | %"+str(sysmod2Size)+"s | %21s | %5s | %7s ") % (time,modsys1,modsys2,ip+":"+port,cnt,suc_cnt)) +
            ("| %6.2f%% | %7.1fms | %7.1fms | %7.1fms\n" % (successPercent, meanTimeMs, successMeanTimeMs, failMeanTimeMs) ) )

def PrintHead():

    global sysmodSizeMax 
    global sysmod1Size 
    global sysmod2Size 

    sys.stdout.write(("%20s | %"+str(sysmod1Size)+"s | %"+str(sysmod2Size)+"s | %21s | %5s | %7s | %7s | %9s | %9s | %9s\n" )
                    % ("time","sys1:mod1"
                                                      ,"sys2:mod2"
                                                                           ,"ip:port","cnt","suc_cnt"
                                                                                                  ,"suc(%)"
                                                                                                         ,"mean_time"
                                                                                                               ,"suc_mean"
                                                                                                                   ,"fail_mean"
                        ))
    # sys.stdout.write("time\tsys1\tmod1\tsys2\tmod2\tip\tport|cnt\tsuccess_cnt\ttime\tsuccess_time|cnt\tsuccess_percent\tmean_time\tsuccess_mean_time\tfail_mean_time\n")

Entry()

