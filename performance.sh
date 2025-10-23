#!/bin/bash  
# 服务器性能监控脚本
# 功能：监控服务器进程的CPU、内存使用情况，检查端口监听状态

# 获取进程ID函数
function GetPID #User #Name 
{ 
    PsUser=$1 
    PsName=$2 
    pid=`ps -u $PsUser|grep $PsName|grep -v grep|grep -v vi|grep -v dbx\n |grep -v tail|grep -v start|grep -v stop |sed -n 1p |awk '{print $1}'` 
    echo $pid 
}

# 获取服务器进程PID
PID=`GetPID root server` 
echo -e "server进程 PID: $PID .\n"

# 检查进程是否存在
if [ "-$PID" == "-" ] 
then 
{ 
    echo "The process does not exist."
} 
fi

# 记录监控时间
date +"%Y-%m-%d %H:%M:%S" >> server.log

# 获取进程CPU使用率函数
function GetCpu 
{ 
   CpuValue=`ps -p $1 -o pcpu |grep -v CPU` 
   echo $CpuValue 
}

# 检查CPU使用率是否超过阈值
function CheckCpu 
{ 
    PID=$1 
    cpu=`GetCpu $PID` 
    if [ $cpu > 90 ] 
    then 
    { 
        echo "server进程 CPU 利用率 is larger than 90%:$cpu %."
    } 
    fi 
    echo $cpu
}

# 获取进程CPU使用率
pcpu=`GetCpu $PID`

# 获取进程内存使用量函数
function GetMem 
{ 
    MEMUsage=`ps -o vsz -p $1|grep -v VSZ` 
    (( MEMUsage /= 1000))  # 转换为MB
    echo $MEMUsage 
}

# 获取进程内存使用量
mem=`GetMem $PID` 

# 输出进程资源使用情况
echo -e "=====server进程 CPU 利用率: $pcpu %.======\n=====server进程内存使用量:$mem M.  ======\n" 
echo -e "=====server进程 CPU 利用率: $pcpu %.======\n=====server进程内存使用量:$mem M.  ======\n" >> server.log

# 检查内存使用是否超过阈值
if [ $mem -gt 500 ] 
then 
{ 
    echo "server进程内存使用量超过 500M"
} 
fi

# 检查端口监听状态函数
function Listening 
{ 
    TCPListeningnum=`netstat -an | grep ":$1 " | awk '$1 == "tcp" && $NF == "LISTEN" {print $0}' | wc -l` 
    UDPListeningnum=`netstat -an|grep ":$1 " |awk '$1 == "udp" && $NF == "0.0.0.0:*" {print $0}' | wc -l` 
    (( Listeningnum = TCPListeningnum + UDPListeningnum )) 
    if [ $Listeningnum == 0 ] 
    then 
    { 
        echo "0"
    } 
    else 
    { 
        echo "1"
    } 
    fi 
}

# 检查服务器端口10222是否在监听
isListen=`Listening 10222` 

# 统计客户端进程数量
Runnum=`ps -ef | grep -v vi | grep -v tail | grep "[ /]client" | grep -v grep | wc -l`

# 输出连接状态信息
if [ $isListen -eq 1 ] 
then 
{ 
    echo -e "**端口 10222 在监听**\n**client进程正在运行的个数: $Runnum**\n"
} 
else 
{ 
    echo "**端口 10222 没有在监听**\n**client进程正在运行的个数: $Runnum**\n"
} 
fi

# 获取系统CPU负载函数
function GetSysCPU 
{ 
   CpuIdle=`vmstat 1 5 |sed -n '3,$p' |awk '{x = x + $15} END {print x/5}' |awk -F. '{print $1}'`
   CpuNum=`echo "100-$CpuIdle" | bc` 
   echo $CpuNum 
}

# 获取系统内存使用量函数
function GetSysMem 
{   
  Mem=`free -m | grep '内存' | awk '{print $3}'`  
  echo $Mem  
} 

# 获取系统资源使用情况
cpu=`GetSysCPU` 
mem=`GetSysMem`

# 检查系统CPU使用率
if [ $cpu -gt 90 ] 
then 
{ 
    echo "CPU 利用率超过 90%"
} 
fi

# 输出系统资源使用情况
echo -e "=====系统 CPU 利用率: $cpu %.\t  ======\n=====系统内存使用量: $mem M.\t  ======\n" 
echo -e "=====系统 CPU 利用率: $cpu %.\t  ======\n=====系统内存使用量: $mem M.\t  ======\n" >> server.log