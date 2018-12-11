#!/bin/bash
#检测恶意进程，在5s内如果某一进程的CPU或者内存占用超过50%，视为恶意进程

pid=`ps -aux | awk '{if($3>50.0 || $4>50.0) print $2}'`    #pid代表第一次检测的时候，CPU或者MEM超过50%的进程id

if [[ $pid != "" ]];then
    sleep 5;
    for i in $pid;do 
        ps -p $i -u | tail -n 1 | awk -v time=`date +%Y-%m-%d__%T` '
        {if($3>50.0 || $4>50.0) 
            {printf("%s\t%-50s\t%s\t%s\t%s%%\t%s%%\n", time, $11, $2, $1, $3, $4)};
        }'
    done
fi
