#!/bin/bash
#该脚本为获取一些内存信息
#输出当前计算机的时间、总内存、剩余内存、内存占用率和占用百分比动态平均值

if [ ! "$1" ];then
    echo [ERROR]: Missing parameter
    exit
fi

if [ `echo "$1 > 100" | bc` -eq 1 -o `echo "$1 <= 0" | bc` -eq 1 ];then
    echo [WARNING]: Parameter crossing
    exit
fi

last=$1                                                                       #last代表上一个动态平均值,由程序传入这个参数
date=`date +"%Y-%m-%d__%T"`                                                   #date代表当前时间
arr_mes=(`free -m | awk ' NR == 2 {print $2 " " $3} '`)                       #arr_mes数组存放一些内存信息
total=${arr_mes[0]}                                                           #total代表总内存,单位是M
used=${arr_mes[1]}                                                            #used代表当前占用内存,单位是M
remaining=$[$total-$used]                                                     #remaining代表内存剩余量,单位是M
current=`echo "scale=1;${used}*100/${total}" | bc`                            #current代表当前内存占用率
dynamic_average=`echo "scale=1;0.3*${last}+0.7*${current}" | bc`              #dynamic_average代表占用百分比动态平均值
echo $date ${total}M ${remaining}M ${current}% ${dynamic_average}%
