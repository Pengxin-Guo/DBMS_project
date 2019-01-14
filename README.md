### DBMS_project（分布式监控系统）

#### 一.项目说明

1. `shell`脚本

   - `CPULog.sh`：获取当前设备的`CPU`信息；
   - `DiskLog.sh`：获取当前当前设备的磁盘信息；
   - `MemLog.sh`：获取当前设备的内存信息；
   - `SysInfo.sh`：获取当前设备的一些系统运行状况信息；
   - `Users.sh`：获取当前设备的用户信息；
   - `MPD.sh`：用于检测恶意进程。

2. `client`端

   *功能说明*：

   - 开一个线程，执行心跳函数，告诉`master`端自己上线了，这个心跳函数是死循环，一直在执行，不过会睡一会，避免`client`端上线的时候`master`端却不在线，导致没有连接上；
   - 开六个线程，运行上述六个`shell`脚本，并将运行脚本得到的信息写到本地文件中，运行脚本的函数也是死循环，隔一段时间运行一下脚本，将得到的信息写入相关文件中；如果在运行脚本过程中检测到了警报信息，会立即将警报信息传给`master`端，而不会等`master`端连自己了再传；
   - 主函数中开一个监听端口，用来让`master`连接，一旦`master`主动连接自己，便将运行脚本得到的存储在本地文件中的信息传给`master`端，再将文件清空；

   *细节描述*：

   - 主函数开的监听端口是一个长连接，即这个端口一直监听着，等着`master`端来连自己；
   - 一旦有`master`端主动连接了自己，`client`端会再开一个短连接，用来传文件；
   - 心跳那个连接也是短连接，即这个连接只是用来告诉`master`端自己上线的，连接过后就会断掉；
   - 发送报警信息这个连接也是短连接，即检测到有报警信息，会连接上`master`端，发送报警信息，发送完毕后这个连接就会断掉；

3. `master`端

   *功能说明*：

   - 首先初始化一下本地链表，将自己要监控的机器加到链表里面，开了五个链表，平均存储这些机器，之后这五个链表是被五个线程并行的处理的；
   - 用五个线程并行遍历这五个链表，判断是否可以连上这些机器，连接不上的就从链表中删除，连接上的话就接收从`client`发送过来的文件信息；这五个线程里面是死循环，一直在遍历这五个链表；
   - 用一个线程来接收警报信息；这个线程里面开一个监听端口，一直监听着，如果有`client`端连自己了，就接收`client`端发来的警报信息，存储到相应文件中，接受完信息后就断开这个连接；这个端口是一直开着的，一直处于监听状态；
   - 主函数中开一个监听端口，每当有`client`端连自己的时候，会判断它是否已经被加在链表里面了，如果没有的话就把它加在最短的链表里面，保证每个链表的长度基本相同；

   *细节描述*：

   - 接收警报信息的那个端口是一直开着的，即一直监听着，如果`client`端检测到了警报信息，会通过这个端口和`master`端建立一个短连接来传输警报信息；
   - 主函数的那个端口也是一直处于监听状态的，用来接收`client`端的心跳；
   - 收文件的连接是短连接，遍历链表过程中，一旦可以连上`client`端，边通过这个短连接接收`client`端发来的文件信息，接受完信息之后这个连接就会断开；

#### 二.项目上线

- 首先，将`master`端做成后台进程（守护进程），即将`master`的所有代码放在`fork`里面；

- 写`start`脚本和`stop`脚本，放在了`/usr/bin/gpx_pihealth`下面，

  `gpx_pihealthd.start`代码如下：

  ```bash
  #!/bin/bash
  
  if [[ ! -e /etc/gpx_pihealth.pid ]]; then
         touch /etc/gpx_pihealth.pid
  fi       
  
  source /etc/gpx_pihealth.conf
  
  pre_pid=`cat /etc/gpx_pihealth.pid`
  
  if test -n $pre_pid ;then
          ps -ef | grep -w ${pre_pid} | grep master > /dev/null
          if [[ $? == 0 ]]; then
              echo "Pihealthd has already started."
  	    exit 0
          else
              echo "Pihealthd is starting."
  	    /home/gpx//Github/DBMS_project/master/master
  	    echo "Pihealthd started."
          fi
  else 
  	    echo "Pihealthd is starting."
  	    /home/gpx/Github/DBMS_project/master/master 
  	    echo "Pihealthd start."
  fi
  ```

  `gpx_pihealthd.stop`代码如下：

  ```bash
  #!/bin/bash
  
  echo "Pihealthd is stoping."
  pid=`cat /etc/gpx_pihealth.pid`
  kill $pid
  echo "Pihealthd stop."
  ```

- 设置系统`service`，在`/lib/systemd/system`下面写一个`gpx_pihealthy.service `，代码如下：

  ```bash
  [Unit]
  Description=gpx_pihealthd-1.0
  After=syslog.target network.target remote-fs.target nss-lookup.target
  
  [Service]
  Type=forking
  ExecStart=/usr/bin/gpx_pihealth/gpx_pihealthd.start
  ExecStop=/usr/bin/gpx_pihealth/gpx_pihealthd.stop
  
  [Install]
  WantedBy=multi-user.target
  ```

  之后，启动服务代码：`systemctl start gpx_pihealthy.service`，

  ​	查看服务状态代码：`systemctl status gpx_pihealthy.service`，

  ​	停止服务代码：`systemctl stop gpx_pihealthy.service`。

  注：如果修改了`gpx_pihealthy.service`之后，需要重新加载，命令如下`stemctl daemon-reload`。

- 设置开机自启：`systemctl enable gpx_pihealthy.service`，

  关闭开机自启：`systemctl disable gpx_pihealthy.service`。