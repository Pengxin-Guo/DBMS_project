#!/bin/bash 
#该脚本为获取一些系统运行状况信息

date=`date +"%Y-%m-%d__%T"`                                                                            #date代表当前时间
hostname=`uname -n`                                                                                    #hostname代表主机名
osname=`uname -o`                                                                                      #osname代表OS版本
kernel_version=`uname -a | cut -d " " -f 3`                                                            #kernel_version代表内核版本
run_time=`uptime -p | tr " " "_"`                                                                      #run_time代表运行时间
aver_load=`cat /proc/loadavg | cut -d " " -f 1-3`                                                      #aver_load代表平均负载
eval `df -m | grep "^/dev" | awk '{printf("arr_total_disk["NR"]=%d;arr_used_disk["NR"]=%d\n",$2,$3)}'`
total_disk=0                                                                                           #total_disk代表磁盘总量,单位是M
used_disk=0                                                                                            #used_disk代表磁盘占用量,单位是M
for i in `seq 1 ${#arr_total_disk[@]}`;do
    total_disk=$[$total_disk+${arr_total_disk[$i]}]
    used_disk=$[$used_disk+${arr_used_disk[$i]}]
done
percent_disk=$[$used_disk*100/$total_disk]                                                             #percent_disk代表磁盘已用%
eval `free -m | awk 'NR == 2 {printf("total_MEM=%d;used_MEM=%d\n",$2,$3)}'`                            #total_MEM代表内存大小，used_MEM代表已用内存，单位均为M
percent_MEM=$[$used_MEM*100/$total_MEM]                                                                #percent_MEM代表内存已用%
CPU_temperature=`sensors | awk ' NR == 3 {print $2}' | tr -c [0-9,\.] " "`                             #CPU_temperature代表CPU温度
if [ $percent_disk -lt 80 ];then
    disk_level="normal"                                                                                #disk_level代表磁盘报警级别
elif [ $percent_disk -lt 90 ];then
    disk_level="note"
else
    disk_level="warning"
fi

if [ $percent_MEM -lt 70 ];then
    MEM_level="normal"                                                                                 #MEM_level代表内存报警级别
elif [ $percent_MEM -lt 80 ];then
    MEM_level="note"
else
    MEM_level="warning"
fi

if [ `echo "$CPU_temperature < 50"|bc` -eq 1 ] ; then
    CPU_level="normal"                                                                                 #CPU_level代表CPU报警级别
elif [ `echo "$CPU_temperature < 70"|bc` -eq 1 ];then
    CPU_level="note"
else
    CPU_level="warning"
fi
echo $date $hostname $osname $kernel_version $run_time $aver_load $total_disk ${percent_disk}% $total_MEM ${percent_MEM}% $CPU_temperature $disk_level $MEM_level $CPU_level
