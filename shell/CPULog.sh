#!/bin/bash 
#该脚本主要获取一些CPU信息 

load=`cat /proc/loadavg | cut -d " " -f 1-3`                                 #load代表3个平均负载
#cat /proc/loadavg 输出5个值
#前三个值分别代表系统5分钟、10分钟、15分钟前的平均负载
#第四个值的分子是正在运行的进程数，分母为总进程数
#第五个值是最近运行的进程id
arr_cpu1=(`cat /proc/stat | head -n 1 | cut -d " " -f 2-`)                   #arr_cpu1代表时间1下的CPU信息
sleep 0.5                                                                    #CPU等待0.5秒再执行
arr_cpu2=(`cat /proc/stat | head -n 1 | cut -d " " -f 2-`)                   #arr_cpu2代表时间2下的CPU信息 
date=`date +"%Y-%m-%d__%T"`                                                  #date代表当前时间，date之所以不能放在第一行是因为程序执行过程中有sleep 0.5这个语句
total_cup1=0                                                                 #total_cup1代表截止到时间1的CPU总使用时间
total_cpu2=0                                                                 #total_cpu2代表截止到时间2的CPU总使用时间 
for i in ${arr_cpu1[*]};do
    total_cup1=$[$total_cup1+$i]
done

for j in ${arr_cpu2[*]};do
    total_cpu2=$[$total_cpu2+$j]
done

total=$[$total_cpu2-$total_cup1]                                             #total代表CPU在时间1和时间2之间的使用时间

free=$[${arr_cpu2[3]}-${arr_cpu1[3]}]                                        #free代表CPU在时间1和时间2之间的空闲时间
percent_free=`echo "scale=2;${free}*100/${total}" | bc`                      #percent_free代表CPU空闲率
percent_use=`echo "scale=2;100-$percent_free" | bc`                          #percen_use代表CPU占用率
CPU_temperature=`sensors | awk 'NR == 3 {print $2}' | cut -d "+" -f 2`       #CPU_temperature代表CPU温度
CPU_temperature_num=`echo $CPU_temperature | tr -c [0-9,\.] " "`             #CPU_temperature_num代表不带单位的CPU温度
level=""                                                                     #level代表当前CPU温度警报级别
if [ `echo "$CPU_temperature_num < 50" | bc` -eq 1 ];then
    level="normal"
elif [ `echo "$CPU_temperature_num < 70" | bc` -eq 1 ];then
    level="note"
else
    level="warning"
fi

echo $date $load $percent_use $CPU_temperature $level
