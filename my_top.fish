#!/usr/bin/fish
# 服务器性能监控脚本 (Fish版本)
# 功能：监控服务器进程的CPU、内存使用情况，检查端口监听状态

# 配置参数
set MONITOR_INTERVAL 5  # 监控间隔（秒）
set MAX_ITERATIONS 0  # 最大循环次数（0表示无限循环）

# 获取进程ID函数
function GetPID
    set PsUser $argv[1]
    set PsName $argv[2]
    set pid (ps -u $PsUser | grep $PsName | grep -v grep | grep -v vi | grep -v dbx | grep -v tail | grep -v start | grep -v stop | sed -n 1p | awk '{print $1}')
    echo $pid
end

# 检查端口监听状态函数（使用ss命令替代netstat）
function Listening
    set port $argv[1]
    # 使用ss命令检查TCP监听
    set TCPListeningnum (ss -tln | grep ":$port " | wc -l 2>/dev/null || echo "0")
    # 使用ss命令检查UDP监听
    set UDPListeningnum (ss -uln | grep ":$port " | wc -l 2>/dev/null || echo "0")
    
    # 确保数值有效
    if test -z "$TCPListeningnum"
        set TCPListeningnum "0"
    end
    if test -z "$UDPListeningnum"
        set UDPListeningnum "0"
    end
    
    set Listeningnum (math "$TCPListeningnum + $UDPListeningnum" 2>/dev/null || echo "0")
    
    if test $Listeningnum -eq 0
        echo "0"
    else
        echo "1"
    end
end

# 获取系统CPU负载函数
function GetSysCPU
    set CpuIdle (vmstat 1 5 | sed -n '3,$p' | awk '{x = x + $15} END {print x/5}' | awk -F. '{print $1}' 2>/dev/null || echo "100")
    if test -z "$CpuIdle"
        set CpuIdle "100"
    end
    set CpuNum (math "100 - $CpuIdle" 2>/dev/null || echo "0")
    echo $CpuNum
end

# 获取系统内存使用量函数
function GetSysMem
    # 适配不同系统的free命令输出
    set Mem (free -m | grep -E '内存|Mem' | awk '{print $3}' 2>/dev/null || echo "0")
    if test -z "$Mem"
        set Mem "0"
    end
    echo $Mem
end

# 主监控循环
function MonitorLoop
    set iteration 0
    
    while true
        set iteration (math "$iteration + 1")
        
        # 显示当前监控轮次和时间
        echo -e "\n=== 监控轮次: $iteration, 时间: "(date "+%Y-%m-%d %H:%M:%S")" ==="
        
        # 获取服务器进程PID
        set PID (GetPID yuye server)
        echo -e "server进程 PID: $PID"
        
        # 检查进程是否存在
        if test -z "$PID"
            echo "The process does not exist."
            set pcpu "N/A"
            set mem "N/A"
        else
            # 记录监控时间
            date +"%Y-%m-%d %H:%M:%S" >> server.log
            
            # 获取进程CPU使用率
            function GetCpu
                if test -n "$argv[1]"
                    set CpuValue (ps -p $argv[1] -o pcpu | grep -v CPU | tr -d ' ' 2>/dev/null || echo "0")
                    echo $CpuValue
                else
                    echo "0"
                end
            end
            
            # 获取进程内存使用量函数
            function GetMem
                if test -n "$argv[1]"
                    set MEMUsage (ps -o vsz -p $argv[1] | grep -v VSZ | tr -d ' ' 2>/dev/null || echo "0")
                    if test -n "$MEMUsage" -a "$MEMUsage" != "N/A"
                        set MEMUsage (math "$MEMUsage / 1000" 2>/dev/null || echo "0")  # 转换为MB
                    else
                        set MEMUsage "0"
                    end
                    echo $MEMUsage
                else
                    echo "0"
                end
            end
            
            # 获取进程资源使用情况
            set pcpu (GetCpu $PID)
            set mem (GetMem $PID)
            
            # 输出进程资源使用情况
            echo -e "=====server进程 CPU 利用率: $pcpu %.======"
            echo -e "=====server进程内存使用量: $mem M.======"
            echo -e "=====server进程 CPU 利用率: $pcpu %.======" >> server.log
            echo -e "=====server进程内存使用量: $mem M.======" >> server.log
            
            # 检查内存使用是否超过阈值
            if test -n "$mem" -a "$mem" != "N/A"
                if test $mem -gt 500
                    echo "⚠️ server进程内存使用量超过 500M"
                end
            end
            
            # 检查CPU使用率是否超过阈值
            if test -n "$pcpu" -a "$pcpu" != "N/A"
                if test $pcpu -gt 90
                    echo "⚠️ server进程 CPU 利用率超过 90%: $pcpu %"
                end
            end
        end
        
        # 检查服务器端口10222是否在监听
        set isListen (Listening 10222)
        
        # 统计客户端进程数量
        set Runnum (ps -ef | grep -v vi | grep -v tail | grep "[ /]client" | grep -v grep | wc -l 2>/dev/null || echo "0")
        
        # 输出连接状态信息
        if test "$isListen" = "1"
            echo -e "✅ 端口 10222 在监听"
            echo -e "📊 client进程正在运行的个数: $Runnum"
        else
            echo -e "❌ 端口 10222 没有在监听"
            echo -e "📊 client进程正在运行的个数: $Runnum"
        end
        
        # 获取系统资源使用情况
        set cpu (GetSysCPU)
        set mem (GetSysMem)
        
        # 检查系统CPU使用率
        if test -n "$cpu" -a "$cpu" != "N/A"
            if test $cpu -gt 90
                echo "⚠️ 系统 CPU 利用率超过 90%"
            end
        end
        
        # 输出系统资源使用情况
        echo -e "=====系统 CPU 利用率: $cpu %.======"
        echo -e "=====系统内存使用量: $mem M.======"
        echo -e "=====系统 CPU 利用率: $cpu %.======" >> server.log
        echo -e "=====系统内存使用量: $mem M.======" >> server.log
        
        # 检查是否达到最大循环次数
        if test $MAX_ITERATIONS -gt 0 -a $iteration -ge $MAX_ITERATIONS
            echo -e "\n🎯 已达到最大监控次数 ($MAX_ITERATIONS 次)，监控结束"
            break
        end
        
        # 等待指定间隔
        echo -e "⏰ 等待 $MONITOR_INTERVAL 秒后继续监控...\n"
        sleep $MONITOR_INTERVAL
    end
end

# 显示监控配置
echo "🚀 开始服务器性能监控"
echo "📊 监控间隔: $MONITOR_INTERVAL 秒"
if test $MAX_ITERATIONS -gt 0
    echo "🔄 最大循环次数: $MAX_ITERATIONS"
else
    echo "🔄 无限循环监控 (按 Ctrl+C 停止)"
end
echo "📝 日志文件: server.log"
echo ""

# 启动监控循环
MonitorLoop