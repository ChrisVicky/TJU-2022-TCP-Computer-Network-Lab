# -*- coding: UTF-8 -*-
import time
import pdb
import socket
import sys
from fabric import Connection
import signal
import os

TEST_USER = 'vagrant'
TEST_PASS = 'vagrant'
EXITING = False

logfile = "./log.log"
if os.path.exists(logfile):
    os.remove(logfile)

def log(string):
    print(string)
    with open(logfile, 'a') as f:
        f.write(string)
        f.write('\n')


# 输入 带宽 延迟 丢包率
rate = 100 
delay = 300
delay_distro = 50
loss = 10
if len(sys.argv)>=2:
    rate = int(sys.argv[1])  
    delay = int(sys.argv[2])
    delay_distro = int(sys.argv[3])
    loss = int(sys.argv[4])
log("\n即将使用如下网络状态进行数据传输: rate:%dMbps delay:%dms delay-distro:%d loss:%d%% \n"%(rate, delay, delay_distro, loss)) 

def my_exit(signum, frame):
    global EXITING
    if EXITING==True:
        log('\n正在退出测试程序, 请稍等, 不需要多次停止\n')
        return
        
    EXITING = True
    log('\n正在退出测试程序, 请稍等\n')
    with Connection(host="172.17.0.3", user=TEST_USER, connect_kwargs={'password':TEST_PASS}) as conn:
        log("[退出测试] 结束抓包进程")
        stop_tcpdump_cmd = 'sudo pkill -f "tcpdump -i enp0s8 -w /home/vagrant/server.pcap udp"'
        rst = conn.run(stop_tcpdump_cmd, timeout=10)
        if (rst.failed):
            log('[退出测试] 这里为什么会出错')
        
        log("[退出测试] 结束客户端和服务端进程运行")
        stop_server_cmd = 'sudo pkill -f "/vagrant/tju_tcp/test/rdt_server"'
        stop_client_cmd = 'sudo pkill -f "/vagrant/tju_tcp/test/rdt_client"'
        try:
            rst = conn.local(stop_client_cmd)
            if (rst.failed):
                log("[退出测试] 关闭本地测试程序失败")
        except Exception as e:
            pass 
        try:
            rst = conn.run(stop_server_cmd)
            if (rst.failed):
                log("[退出测试] 关闭服务端测试程序失败")            
        except Exception as e: 
            pass 
        
        log("[退出测试] 重置网络环境")    
        reset_networf_cmd = 'sudo tcset enp0s8 --rate 100Mbps --delay 20ms --overwrite'
        rst = conn.run(reset_networf_cmd)
        if (rst.failed):
            log("[退出测试程序] 重置服务端网络失败")    
        rst = conn.local(reset_networf_cmd)
        if (rst.failed):
            log("[退出测试程序] 重置客户端网络失败")

    exit()

def main():
    
    if socket.gethostname() == "server":
        log("只能在client端运行自动测试")
        exit()

    signal.signal(signal.SIGINT, my_exit)
    signal.signal(signal.SIGTERM, my_exit)
        
    log("====================================================================")
    log("============================开始测试================================")
    log("====================================================================")
    with Connection(host="172.17.0.3", user=TEST_USER, connect_kwargs={'password':TEST_PASS}) as conn:
        
        # 编译提交的源码
        log("[自动测试] 编译提交源码")
        log("> cd /vagrant/tju_tcp && make")
        rst = conn.run("cd /vagrant/tju_tcp && make", timeout=10)
        if (rst.failed):
            log('[自动测试] 编译提交源码错误 停止测试')
            log('{"scores": {"establish_connection": 0}}')
            exit()
        log("")
        
        # 判断是否使用用户提交的新Makefile
        if os.path.exists("/vagrant/tju_tcp/test_Makefile"):
            log("[自动测试] 检测到用户提供了自己的测试Makefile, 使用用户的测试Makefile替换自带的版本")
            log("> cp /vagrant/tju_tcp/test_Makefile /vagrant/tju_tcp/test/Makefile")
            rst = conn.local("cp /vagrant/tju_tcp/test_Makefile /vagrant/tju_tcp/test/Makefile", timeout=10)
            if (rst.failed):
                log("[自动测试] 移动测试Makefile失败")
                exit()
        log("")

        # 编译测试源码
        log("[自动测试] 编译测试源码")
        log("> cd /vagrant/tju_tcp/test && make")
        rst = conn.run("cd /vagrant/tju_tcp/test && make", timeout=10)
        if (rst.failed):
            log('[自动测试] 编译测试源码错误 停止测试')
            log('{"scores": {"establish_connection": 0}}')
            exit()    
        log("")


        log("============================ 抓包并绘制SeqNum随Time变化图 ================================")

        # 保证没有上一次的抓包进程在运行
        stop_tcpdump_cmd = 'sudo pkill -f "tcpdump -i enp0s8 -w /home/vagrant/server.pcap udp"' # pkill -f x kill含x的所有进程
        rst = conn.run(stop_tcpdump_cmd, timeout=10)
        if (rst.failed):
            log('[抓包并绘图] 这里为什么会出错')
        
        # 保证没有之前的客户端和服务端进程运行
        stop_server_cmd = 'sudo pkill -f "/vagrant/tju_tcp/test/rdt_server"'
        stop_client_cmd = 'sudo pkill -f "/vagrant/tju_tcp/test/rdt_client"'
        try:
            rst = conn.local(stop_client_cmd)
            if (rst.failed):
                log("[抓包并绘图] 关闭本地测试程序失败")
        except Exception as e:
            pass 
        try:
            rst = conn.run(stop_server_cmd)
            if (rst.failed):
                log("[抓包并绘图] 关闭服务端测试程序失败")            
        except Exception as e: 
            pass 

        # 保证建立连接的网络环境
        reset_networf_cmd = 'sudo tcset enp0s8 --rate 100Mbps --delay 20ms --overwrite' 
        conn.run(reset_networf_cmd)
        if (rst.failed):
            log("[数据传输测试] 重置服务端网络失败")
        conn.local(reset_networf_cmd)
        if (rst.failed):
            log("[数据传输测试] 重置客户端网络失败")

        log("[抓包并绘图] 清理上一次的抓包结果")
        erase_cmd = "sudo rm -f ~/server.pcap"
        rst = conn.run(erase_cmd, timeout=10)
        if (rst.failed):
            log('[抓包并绘图] 清理上一次的抓包结果错误')
            exit()
        


        # 进行数据传输前，先开始抓包
        log("[抓包并绘图] 开始在服务端抓包")
        start_tcpdump_cmd = "sudo tcpdump -i enp0s8 -w /home/vagrant/server.pcap udp > /dev/null 2> /dev/null < /dev/null & " 
        # > /dev/null 忽略命令正常执行的输出; 2> /dev/null 忽略错误输出; < /dev/null 避免脚本等待输入; & 设置进程为后台进程
        rst = conn.run(start_tcpdump_cmd, timeout=10)
        if (rst.failed):
            log('[抓包并绘图] 服务端开启抓包失败')
            conn.run(stop_tcpdump_cmd, timeout=10)
            exit()

        # 先开启服务端等待接收数据，再开启客户端开始发送数据
        log("[抓包并绘图] 开启服务端和客户端")
        start_server_cmd = 'nohup sudo /vagrant/tju_tcp/test/rdt_server > /vagrant/tju_tcp/test/server.log 2>&1 < /dev/null & '
        start_client_cmd = 'nohup sudo /vagrant/tju_tcp/test/rdt_client > /vagrant/tju_tcp/test/client.log 2>&1 < /dev/null & '
        # nohup 用于在系统后台不挂断地运行命令，退出终端不会影响程序的运行
        # 2>&1代表将stderr重定向到文件描述符为1的文件(即/dev/stdout)中
        rst = conn.run(start_server_cmd, pty=False)
        if (rst.failed):
            log("[抓包并绘图] 无法运行服务端测试程序")
            exit()
        rst = conn.local(start_client_cmd, pty=False)
        if (rst.failed):
            log("[抓包并绘图] 无法运行客户端测试程序")
            exit()
        
        # 等待建立连接
        log("[抓包并绘图] 等待6s建立连接")
        time.sleep(6)
        
        # 连接建立 模拟网络延迟 丢包
        set_networf_cmd = 'sudo tcset enp0s8 --rate %dMbps --delay %dms --delay-distro %d --loss %d%% --overwrite'%(rate, delay, delay_distro, loss)
        log("[抓包并绘图] 建立连接后  设置双方的网络通讯速率 丢包率 和延迟: %s"%set_networf_cmd)
        rst = conn.run(set_networf_cmd)
        if (rst.failed):
            log("[抓包并绘图] 无法设置服务端网络")
            exit()
        rst = conn.local(set_networf_cmd)
        if (rst.failed):
            log("[抓包并绘图] 无法设置客户端网络")
            exit()

        # 等待60s的数据传输
        log("[抓包并绘图] 等待60s 进行双方通信")
        time.sleep(60)
        
        # 停止抓包
        log("[抓包并绘图] 停止抓包")
        stop_tcpdump_cmd = 'sudo pkill -f "tcpdump -i enp0s8 -w /home/vagrant/server.pcap udp"'
        rst = conn.run(stop_tcpdump_cmd, timeout=10)
        if (rst.failed):
            log('[抓包并绘图] 这里为什么会出错')

        # 关闭测试服务端和客户端
        log("[抓包并绘图] 关闭双端")
        stop_server_cmd = 'sudo pkill -f "/vagrant/tju_tcp/test/rdt_server"'
        stop_client_cmd = 'sudo pkill -f "/vagrant/tju_tcp/test/rdt_client"'
        try:
            rst = conn.local(stop_client_cmd)
            if (rst.failed):
                log("[抓包并绘图] 关闭本地测试程序失败")
        except Exception as e:
            pass 
        try:
            rst = conn.run(stop_server_cmd)
            if (rst.failed):
                log("[抓包并绘图] 关闭服务端测试程序失败")            
        except Exception as e: 
            pass 

        # 取消延迟 丢包等网络配置
        log("[抓包并绘图] 取消网络设置")
        reset_networf_cmd = 'sudo tcset enp0s8 --rate 100Mbps --delay 20ms --overwrite'
        conn.run(reset_networf_cmd)
        if (rst.failed):
            log("[数据传输测试] 重置服务端网络失败")
        conn.local(reset_networf_cmd)
        if (rst.failed):
            log("[数据传输测试] 重置客户端网络失败")


        log("[ test ] cp ./server.event.trace to upper file")
        cp_trace_cmd = "cp ./server.event.trace /vagrant/tju_tcp/test/server.event.trace"
        rst = conn.run(cp_trace_cmd)
        if(rst.failed):
            log("[ test ] cp Error")

        log("[ test ] cp server.pcap to here")
        cp_trace_cmd = "cp /home/vagrant/server.pcap /vagrant/tju_tcp/test/server.pcap"
        rst = conn.run(cp_trace_cmd)
        if(rst.failed):
            log("[ test ] cp Error")



if __name__ == "__main__":
    main()

        
