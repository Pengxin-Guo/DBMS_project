#!/bin/bash
#该脚本为统计一些用户信息
#输出当前时间、用户总数(非系统用户) 近期活跃用户(3个) 具有ROOT权限用户 当前在线用户_登录IP_TTY

date=`date +"%Y-%m-%d__%T"`                                                                                                             #date代表当前时间
num_users=`cat /etc/passwd | grep "bash" | wc -l`                                                                                       #num_users代表用户总数
active_users=`last | head -n -2 | cut -d " " -f 1 | sort | uniq -c | sort -n -r | head -n 3 | awk '{print $2}' | xargs | tr " " ","`    #active_users代表近期最活跃的3个用户
root_users=`cat /etc/group | grep "sudo" | cut -d ":" -f 4`                                                                             #root_users代表具有root权限的用户
current_name=`who | awk '{print $1 $5 $2 "\n"}' | tr [\(,\)] "_" | xargs | tr " " ","`                                                  #current_name代表当前在线用户
echo $date $num_users [${active_users}] [${root_users}] [${current_name}]
