#!/usr/bin/fish
# @file my_top.fish
# @brief 服务器性能监控脚本 (Fish Shell版本)
# @details 监控服务器进程的CPU、内存使用情况，检查端口监听状态
# @note 使用Fish Shell编写，需要安装fish解释器

# 配置参数
set MONITOR_INTERVAL 5  # 监控间隔（秒）
set MAX_ITERATIONS 0  # 最大循环次数（0表示无限循环）

/**
 * @brief 获取指定进程的PID
 * @param PsUser 进程所属用户名
 * @param PsName 进程名称
 * @return 进程PID
 */
function GetPID
    set PsUser $argv[1]
    set PsName $argv[2]
    # 使用ps命令查找进程，过滤掉无关进程
    set pid (ps -u $PsUser | grep $PsName | grep -v grep | grep -v vi | grep -v dbx | grep -v tail | grep -v start | grep -v stop | sed -n 1p | awk '{print $1}')
    echo $pid
end

/**
 * @brief 检查指定端口是否在监听状态
 * @param port 端口号
 * @return 1-在监听，0-未监听
 */
function Listening
    set port $argv[1]
    # 使用ss命令检查TCP监听状态
    set TCPListeningnum (ss -tln | grep ":$port " | wc -l 2>/dev/null || echo "0")
    # 使用ss命令检查UDP监听状态
    set UDPListeningnum (ss -uln | grep ":$port " | wc -l 2>/dev/null || echo "0")
    
    # 确保数值有效（处理空值情况）
    if test -z "$TCPListeningnum"
        set TCPListeningnum "0"
    end
    if test -z "$UDPListeningnum"
        set UDPListeningnum "0"
    end
    
    # 计算总监听数（TCP + UDP）
    set Listeningnum (math "$TCPListeningnum + $UDPListeningnum" 2>/dev/null || echo "0")
    
    # 返回监听状态
    if test $Listeningnum -eq 0
        echo "0"
    else
        echo "1"
    end
end

/**
 * @brief 获取系统CPU负载百分比
 * @return CPU使用率百分比（0-100）
 */
function GetSysCPU
    # 使用vmstat获取CPU空闲时间，计算使用率
    set CpuIdle (vmstat 1 5 | sed -n '3,$p' | awk '{x = x + $15} END {print x/5}' | awk -F. '{print $1}' 2>/dev/null || echo "100")
    
    # 处理空值情况
    if test -z "$CpuIdle"
        set CpuIdle "100"
    end
    
    # 计算CPU使用率：100 - 空闲率
    set CpuNum (math "100 - $CpuIdle" 2>/dev/null || echo "0")
    echo $CpuNum
end

/**
 * @brief 获取系统内存使用量（MB）
 * @return 内存使用量（MB）
 */
function GetSysMem
    # 使用free命令获取内存使用量，适配不同系统输出
    set Mem (free -m | grep -E '内存|Mem' | awk '{print $3}' 2>/dev/null || echo "0")
    
    # 处理空值情况
    if test -z "$Mem"
        set Mem "0"
    end
    echo $Mem
end

/**
 * @brief 主监控循环函数
 * @details 循环监控服务器进程状态和系统资源
 */
function MonitorLoop
    set iteration 0
    
    # 无限循环监控（可通过MAX_ITERATIONS限制）
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
            # 记录监控时间到日志文件
            date +"%Y-%m-%d %H:%M:%S" >> server.log
            
            # 定义获取进程CPU使用率的内部函数
            function GetCpu
                if test -n "$argv[1]"
                    # 使用ps命令获取进程CPU使用率
                    set CpuValue (ps -p $argv[1] -o pcpu | grep -v CPU | tr -d ' ' 2>/dev/null || echo "0")
                    echo $CpuValue
                else
                    echo "0"
                end
            end
            
            # 定义获取进程内存使用量的内部函数
            function GetMem
                if test -n "$argv[1]"
                    # 使用ps命令获取进程虚拟内存大小（VSZ）
                    set MEMUsage (ps -o vsz -p $argv[1] | grep -v VSZ | tr -d ' ' 2>/dev/null || echo "0")
                    if test -n "$MEMUsage" -a "$MEMUsage" != "N/A"
                        # 转换为MB单位
                        set MEMUsage (math "$MEMUsage / 1000" 2>/dev/null || echo "0")
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
            
            # 输出进程资源使用情况到控制台和日志文件
            echo -e "=====server进程 CPU 利用率: $pcpu %.======"
            echo -e "=====server进程内存使用量: $mem M.======"
            echo -e "=====server进程 CPU 利用率: $pcpu %.======" >> server.log
            echo -e "=====server进程内存使用量: $mem M.======" >> server.log
            
            # 检查内存使用是否超过阈值（500MB）
            if test -n "$mem" -a "$mem" != "N/A"
                if test $mem -gt 500
                    echo "⚠️ server进程内存使用量超过 500M"
                end
            end
            
            # 检查CPU使用率是否超过阈值（90%）
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
        
        # 检查系统CPU使用率是否超过阈值（90%）
        if test -n "$cpu" -a "$cpu" != "N/A"
            if test $cpu -gt 90
                echo "⚠️ 系统 CPU 利用率超过 90%"
            end
        end
        
        # 输出系统资源使用情况到控制台和日志文件
        echo -e "=====系统 CPU 利用率: $cpu %.======"
        echo -e "=====系统内存使用量: $mem M.======"
        echo -e "=====系统 CPU 利用率: $cpu %.======" >> server.log
        echo -e "=====系统内存使用量: $mem M.======" >> server.log
        
        # 检查是否达到最大循环次数
        if test $MAX_ITERATIONS -gt 0 -a $iteration -ge $MAX_ITERATIONS
            echo -e "\n🎯 已达到最大监控次数 ($MAX_ITERATIONS 次)，监控结束"
            break
        end
        
        # 等待指定间隔后继续监控
        echo -e "⏰ 等待 $MONITOR_INTERVAL 秒后继续监控...\n"
        sleep $MONITOR_INTERVAL
    end
end

# 显示监控配置信息
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