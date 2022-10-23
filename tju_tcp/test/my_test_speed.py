# -*- coding: UTF-8 -*-
import time
import socket
import sys
from fabric import Connection
import matplotlib.pyplot as plt
import signal
import sys
import time
import numpy as np
import os
import shutil
from matplotlib.pyplot import MultipleLocator
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
rate = 50
delay = 6
delay_distro = 0
loss = 0
if len(sys.argv)>=2:
    rate = int(sys.argv[1])  
    delay = int(sys.argv[2])
    delay_distro = int(sys.argv[3])
    loss = int(sys.argv[4])
log("\n即将使用如下网络状态进行数据传输: rate:%dMbps delay:%dms delay-distro:%d loss:%d%% \n"%(rate, delay, delay_distro, loss)) 

start_time = 0
end_time = 0
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

def main(INIT_WDS):
    
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
        log(f"> cd /vagrant/tju_tcp && make INIT_WDS={INIT_WDS}")
        rst = conn.run(f"cd /vagrant/tju_tcp && make INIT_WDS={INIT_WDS}", timeout=10)
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
        log(f"> cd /vagrant/tju_tcp/test && make INIT_WDS={INIT_WDS}")
        rst = conn.run(f"cd /vagrant/tju_tcp/test && make INIT_WDS={INIT_WDS}", timeout=10)
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
        log("[抓包并绘图] 等待 300s 进行双方通信")
        time.sleep(300)
        
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




def read_trace(file):
    SEND_dic = {'utctime':[], 'seq':[], 'ack':[], 'flag':[], 'length':[]}
    RECV_dic = {'utctime':[], 'seq':[], 'ack':[], 'flag':[], 'length':[]}
    CWND_dic = {'utctime':[], 'type':[], 'size':[]}
    RWND_dic = {'utctime':[], 'size':[]}
    SWND_dic = {'utctime':[], 'size':[]}
    RTTS_dic = {'utctime':[], 'SampleRTT':[], 'EstimatedRTT':[], 'DeviationRTT':[], 'TimeoutInterval':[]}
    DELV_dic = {'utctime':[], 'seq':[], 'size':[], 'throughput':[]}

    global start_time, end_time 
    with open(file, 'r', encoding='utf-8') as f:
        for num, line in enumerate(f):
            if(line=='\n'): continue # 跳过空行
            if('SEND' not in line and 'RECV' not in line and 'CWND' not in line and 'RWND' not in line 
            and 'SWND' not in line and 'RTTS' not in line and 'DELV' not in line): continue # 跳过非事件行
            line = line.strip('\n')
            line = line.replace('[', '')
            line = line.replace(']', '')
            line_list = line.split(' ')
            info_list = line_list[2:]
            info_list = [item.split(':')[1] for item in info_list]
            
            if line_list[1] == 'SEND':
                SEND_dic['utctime'].append(int(line_list[0]))
                SEND_dic['seq'].append(int(info_list[0]))
                SEND_dic['ack'].append(int(info_list[1]))
                SEND_dic['flag'].append(info_list[2])
                # SEND_dic['length'].append(int(info_list[3]))
            elif line_list[1] == 'RECV':
                RECV_dic['utctime'].append(int(line_list[0]))
                RECV_dic['seq'].append(int(info_list[0]))
                RECV_dic['ack'].append(int(info_list[1]))
                RECV_dic['flag'].append(info_list[2])
                # RECV_dic['length'].append(int(info_list[3]))
            elif line_list[1] == 'CWND':
                CWND_dic['utctime'].append(int(line_list[0]))
                CWND_dic['type'].append(int(info_list[0]))
                CWND_dic['size'].append(int(info_list[1])/1375)
            elif line_list[1] == 'RWND':
                RWND_dic['utctime'].append(int(line_list[0]))
                RWND_dic['size'].append(int(info_list[0])/1375)
            elif line_list[1] == 'SWND':
                SWND_dic['utctime'].append(int(line_list[0]))
                SWND_dic['size'].append(int(info_list[0])/1375)
            elif line_list[1] == 'RTTS':
                if line_list[0] not in RTTS_dic['utctime']: 
                    RTTS_dic['utctime'].append(int(line_list[0]))
                    RTTS_dic['SampleRTT'].append(float(info_list[0]))
                    RTTS_dic['EstimatedRTT'].append(float(info_list[1]))
                    RTTS_dic['DeviationRTT'].append(float(info_list[2]))
                    RTTS_dic['TimeoutInterval'].append(float(info_list[3]))
            elif line_list[1] == 'DELV':
                DELV_dic['utctime'].append(int(line_list[0]))
                DELV_dic['seq'].append(int(info_list[0]))
                DELV_dic['size'].append(int(info_list[1])) 

            if start_time==0:
                start_time = int(line_list[0])
            end_time = int(line_list[0]) - start_time


    SEND_dic['time'] = [item - start_time for item in SEND_dic['utctime']]
    RECV_dic['time'] = [item - start_time for item in RECV_dic['utctime']]
    CWND_dic['time'] = [item - start_time for item in CWND_dic['utctime']]
    RWND_dic['time'] = [item - start_time for item in RWND_dic['utctime']]
    SWND_dic['time'] = [item - start_time for item in SWND_dic['utctime']]
    RTTS_dic['time'] = [item - start_time for item in RTTS_dic['utctime']]
    DELV_dic['time'] = [item - start_time for item in DELV_dic['utctime']]

    SEND_dic['time'] = np.divide(SEND_dic['time'], 1000000) # 单位: s
    RECV_dic['time'] = np.divide(RECV_dic['time'], 1000000)
    CWND_dic['time'] = np.divide(CWND_dic['time'], 1000000)
    RWND_dic['time'] = np.divide(RWND_dic['time'], 1000000)
    SWND_dic['time'] = np.divide(SWND_dic['time'], 1000000)
    RTTS_dic['time'] = np.divide(RTTS_dic['time'], 1000000)
    DELV_dic['time'] = np.divide(DELV_dic['time'], 1000000)
    end_time = end_time / 1000000

    return SEND_dic, RECV_dic, CWND_dic, RWND_dic, SWND_dic, RTTS_dic, DELV_dic


def delete_lines(filename, head,tail):
    fin = open(filename, 'r')
    a = fin.readlines()
    fout = open(filename, 'w')
    b = ''.join(a[head:-tail])
    fout.write(b)


if __name__ == "__main__":
    init_wds_list = [i for i in range(8, 72, 8)]
    if not os.path.exists("./speed.log"):
        rates = []

        for init_wds in init_wds_list:
            log(f"init_wds: {init_wds}")
            main(init_wds)
            FILE_TO_READ = '/vagrant/tju_tcp/test/server.event.trace'
            ## Start Calculating Data
            delete_lines(FILE_TO_READ,0,1) # 删除最后一行（可能 client.event.trace 有问题）
            SEND_dic, RECV_dic, CWND_dic, RWND_dic, SWND_dic, RTTS_dic, DELV_dic = read_trace(FILE_TO_READ)
            interval_t = DELV_dic['time'][-1] - DELV_dic['time'][0]
            total = 0
            for i in DELV_dic['size']:
                total = total + i
            log(f"ave rate: {total}/{interval_t}={total/interval_t}")
            rate_ = total/interval_t
            log(f"{rate_}")
            rates.append(rate_)

        with open("./speed.log", "w") as f:
            f.write(f"{rates}")
            f.write(f"{init_wds_list}")
    else:
        # with open("./speed.log", "r") as f:
        #     init_wds_list = f.readlines()[0]
        #     rates = f.readlines()[1]
        rates = [206374665.68669486, 443121813.9848033, 668958966.0570223, 887279954.5712662, 1127344877.3448787, 1539314081.6452184, 2029715028.010073, 2378121284.1854978]
    rates = [i/1000000 for i in rates] 

    plt.plot(init_wds_list, rates)
    plt.scatter(init_wds_list, rates, c='red')
    for x,y in zip(init_wds_list, rates):
        plt.text(x,y,f"{y:.4f}",ha='right')
    plt.ylabel(f"Throughput / Mbps")
    plt.xlabel(f"Window Size / MSS")
    plt.title(f"Throughtput - Window Size")
    x_major_locator=MultipleLocator(8)
    plt.xlim(-0.5,68)
    ax=plt.gca()
    #ax为两条坐标轴的实例
    ax.xaxis.set_major_locator(x_major_locator)
    plt.legend()
    plt.savefig('/vagrant/tju_tcp/test/wds.png', dpi=600)
    print("绘制成功，图像位于/vagrant/tju_tcp/test/wds.png")






