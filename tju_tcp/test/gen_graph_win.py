#encoding: utf-8
import matplotlib.pyplot as plt
import sys
import time
import numpy as np
import os
import shutil

EVEN = 1 # 0: use time interval, 1: use evenly interval

font_size = 15

logfile='./log.log'
def log(string):
    print(string)
    with open(logfile, 'a') as f:
        f.write(string)
        f.write('\n')

def delete_lines(filename, head,tail):
    fin = open(filename, 'r')
    a = fin.readlines()
    fout = open(filename, 'w')
    b = ''.join(a[head:-tail])
    fout.write(b)


def get_current_time():
    s = str(time.time())
    return s[:s.find('.')]


log("Open ./log.log")
with open('./log.log', 'r') as file:
    lines = file.readlines()
    lines = lines[1]
    config = lines[lines.find(' '):]
    log(f"config {config}")



config = config.replace('%','')
config = config.replace('\n','')
rate = 0.0
t = get_current_time()
t = config.replace(' ','')
t = f"wds{sys.argv[1]}"
start_time = 0
end_time = 0

if not os.path.exists(f"/vagrant/tju_tcp/figure/{t}"):
    os.mkdir(f"/vagrant/tju_tcp/figure/{t}")

def plot_win(time_list, win_list,xstart, xend,  win_type, type=[]):
    # plt.rcParams['figure.figsize'] = (10.0, 5.0)
    if EVEN:
        ll = np.arange(time_list.shape[0]) + xstart
    else:
        ll = time_list
    plt.plot(ll, win_list, color='black')
    plt.xlabel('Time (s)', fontdict={'size':font_size})

    plt.ylabel('%s Window Size (segment)'%win_type, fontdict={'size':font_size})
    plt.title(f"{win_type} {config}")

    if win_type=='Congestion':
        map_color = {0: 'red', 1: 'green', 2:'blue', 3:'cyan'}
        map_label = {0: 'slow start', 1: 'congestion avoidance', 2:'fast retransmit', 3:  'timeout'}
        for item in range(4):
            idx = np.argwhere(np.array(type)==item).reshape(1,-1).tolist()[0]
            plt.scatter(ll[idx], win_list[idx], c=map_color[item], label=map_label[item])
            # plt.scatter(time_list[idx], win_list[idx], c=map_color[item], label=map_label[item])
        plt.legend()

    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    plt.savefig(f"/vagrant/tju_tcp/figure/{t}/{win_type}_WindowSize_VS_Time_{xstart}_{xend}.png", dpi=600)
    log(f"绘制成功, 图像位于/vagrant/tju_tcp/figure/{t}/{win_type}_WindowSize_VS_Time_{xstart}_{xend}.png")
    plt.cla()

def plot_all(cwnd_list, rwnd_list, swnd_list):
    plt.plot(cwnd_list[0], cwnd_list[1], color='red', label='cwnd')
    plt.plot(rwnd_list[0], rwnd_list[1], color='green', label='rwnd')
    plt.plot(swnd_list[0], swnd_list[1], color='blue', label='swnd')
    xstart = min(cwnd_list[0][0], min(rwnd_list[0][0], swnd_list[0][0]))
    xend = max(cwnd_list[0][0], max(rwnd_list[0][0], swnd_list[0][0]))
    plt.title(f"all {config}")

    plt.legend(loc=0, numpoints=1)
    plt.xlabel('Time (s)', fontdict={'size':font_size})
    plt.ylabel('Window Size (segment)', fontdict={'size':font_size})
    plt.legend()                                                    
    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    plt.savefig(f"/vagrant/tju_tcp/figure/{t}/AllWindowSize_VS_Time_{xstart}_{xend}.png", dpi=600)
    log(f"绘制成功, 图像位于/vagrant/tju_tcp/figure/{t}/AllWindowSize_VS_Time_{xstart}_{xend}.png")
    plt.cla()

def plot_rtt(time_list, SampleRTT, EstimatedRTT, DeviationRTT, TimeoutInterval, xstart, xend):
    if EVEN:
        time_list = np.arange(time_list.shape[0])
    plt.plot(time_list, SampleRTT, color='red', label='SampleRTT')
    plt.plot(time_list, EstimatedRTT, color='green', label='EstimatedRTT')
    plt.plot(time_list, DeviationRTT, color='blue', label='DeviationRTT')
    plt.plot(time_list, TimeoutInterval, color='black', label='TimeoutInterval')
    plt.title(f"rtt {config}")
    plt.legend()
    plt.xlabel('Time (s)', fontdict={'size':font_size})
    plt.ylabel('Time (ms)', fontdict={'size':font_size})
    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    plt.savefig(f"/vagrant/tju_tcp/figure/{t}/RTT_VS_Time_{xstart}_{xend}.png", dpi=600)
    log(f"绘制成功, 图像位于/vagrant/tju_tcp/figure/{t}/RTT_VS_Time_{xstart}_{xend}.png")
    plt.cla()

def plot_throughput(time_list, throuput_list, thrp_intv):
    plt.plot(time_list, throuput_list, color='black')
    plt.xlabel('Time (s)', fontdict={'size':font_size})
    plt.ylabel('throuput (bps)', fontdict={'size':font_size})
    plt.ylim(ymin=0, ymax=max(throuput_list)*1.05)
    plt.title(f"throughput {config}")
    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    plt.savefig(f"/vagrant/tju_tcp/figure/{t}/Throuput.png", dpi=600)
    log(f"绘制成功, 图像位于/vagrant/tju_tcp/figure/{t}/Throuput.png [注: 每{thrp_intv:.3f}s计算一次瞬时吞吐率]")
    plt.cla()

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


FILE_TO_READ = '/vagrant/tju_tcp/test/client.event.trace'
FILE_TO_READ_DELV = '/vagrant/tju_tcp/test/server.event.trace'

delete_lines(FILE_TO_READ,0,1) # 删除最后一行（可能 client.event.trace 有问题）
delete_lines(FILE_TO_READ_DELV, 0, 1)
log("正在使用 %s Trace文件绘图"%FILE_TO_READ)
SEND_dic, RECV_dic, CWND_dic, RWND_dic, SWND_dic, RTTS_dic, DELV_dic = read_trace(FILE_TO_READ)

def get_rate_len(time_list, num, start):
    rr = time_list[-1]
    part = time_list[min(num+start-1, len(time_list)-1)]
    return part / rr

        

# 绘图时是否需要间隔 间隔的大小 绘制的数据区间 需要大家根据自己的数据以及绘图效果自行调整, 可参考
intv = 100 
left = 1
iterv = 0.16
cleft = 1
rleft = 1
sleft = 1
clen = 180
while(rate <= 1.0):
    rate = get_rate_len(CWND_dic['time'], clen, cleft)
    interval_time = end_time * rate
    right = len([i for i in CWND_dic['time'][cleft:] if i <= interval_time]) + cleft
    log(f"{cleft}:{right}")
    cwnd_time = CWND_dic['time'][cleft:right]
    cwnd_size = CWND_dic['size'][cleft:right]
    cwnd_type = CWND_dic['type'][cleft:right]
    if len(CWND_dic['utctime']):
        # plot_win(CWND_dic['time'], np.array(CWND_dic['size']), 'Congestion', CWND_dic['type']) # 用全部数据绘图
        # plot_win(CWND_dic['time'][::intv], np.array(CWND_dic['size'][::intv]), 'Congestion', CWND_dic['type'][::intv]) # 间隔100个数据进行绘制
        # plot_win(CWND_dic['time'][:100], np.array(CWND_dic['size'][:100]), 'Congestion', CWND_dic['type'][:100]) # 仅绘制前100个数据点
        plot_win(cwnd_time, np.array(cwnd_size), cleft, right, 'Congestion', cwnd_type) # 仅绘制第100到第200的数据点
    cleft = right + 1
    
    right = len([i for i in RWND_dic['time'][rleft:] if i <= interval_time]) + rleft
    rwnd_size = RWND_dic['size'][rleft:right]
    rwnd_time = RWND_dic['time'][rleft:right]
    if len(RWND_dic['utctime']):
        # plot_win(RWND_dic['time'][::intv], RWND_dic['size'][::intv], 'Receive')
        plot_win(rwnd_time, rwnd_size, rleft, right, 'Receive')
    rleft = right + 1
    
    right = len([i for i in SWND_dic['time'][sleft:] if i <= interval_time]) + sleft
    swnd_time = SWND_dic['time'][sleft:right]
    swnd_size = SWND_dic['size'][sleft:right]
    if len(SWND_dic['utctime']):
        # plot_win(SWND_dic['time'][::intv], SWND_dic['size'][::intv], 'Send')
        plot_win(swnd_time, swnd_size, sleft, right, 'Send')
    sleft = right + 1
    
    rtt_time = RTTS_dic['time'][:]
    rtt_sample = RTTS_dic['SampleRTT'][:]
    rtt_estimate = RTTS_dic['EstimatedRTT'][:]
    rtt_deviate = RTTS_dic['DeviationRTT'][:]
    rtt_timeout = RTTS_dic['TimeoutInterval'][:]
    
    if(len(cwnd_time) and len(rwnd_time) and len(swnd_size)):
        plot_all([cwnd_time, cwnd_size], [rwnd_time, rwnd_size], [swnd_time, swnd_size])
    
    if len(RTTS_dic['utctime']): 
        plot_rtt(rtt_time, rtt_sample, rtt_estimate, rtt_deviate, rtt_timeout, sleft, right)

    break
    
    # 每间隔1s绘制一次吞吐率
if len(DELV_dic['utctime']): 
    thrp_intv = 1 # throughput interval
    time_start = DELV_dic['time'][0]
    intvs = int(DELV_dic['time'][-1])
    thrp_list = []
    for i in range(intvs):
        time_end = time_start+thrp_intv
        mark = (DELV_dic['time']>=time_start) & (DELV_dic['time']<time_end)
        payloads_size = np.array(DELV_dic['size'])[mark]
        DELV_dic['throughput'].append(np.sum(payloads_size)*8/thrp_intv) # 单位: bps
        time_start += thrp_intv
    plot_throughput(range(intvs), DELV_dic['throughput'], thrp_intv)
 


if os.path.exists('SeqNum_VS_Time.png'):
    shutil.copy('SeqNum_VS_Time.png', f"/vagrant/tju_tcp/figure/{t}/SeqNum_VS_Time.png")
    log(f"复制成功, 图像位于/vagrant/tju_tcp/figure/{t}/SeqNum_VS_Time.png")

shutil.copy(FILE_TO_READ, f"/vagrant/tju_tcp/figure/{t}/client.event.trace")
log(f"当前 trace 文件复制到, /vagrant/tju_tcp/figure/{t}/client.event.trace")

shutil.copy(FILE_TO_READ_DELV, f"/vagrant/tju_tcp/figure/{t}/server.event.trace")
log(f"当前 server trace 文件复制到, /vagrant/tju_tcp/figure/{t}/server.event.trace")


# if len(sys.argv)>=2:
# 	FILE_TO_READ = sys.argv[1]
log("正在使用 %s Trace文件绘图"%FILE_TO_READ_DELV)
SEND_dic, RECV_dic, CWND_dic, RWND_dic, SWND_dic, RTTS_dic, DELV_dic = read_trace(FILE_TO_READ_DELV)

# 每间隔1s绘制一次吞吐率
if len(DELV_dic['utctime']): 
    thrp_intv = 1 # throughput interval
    time_start = DELV_dic['time'][0]
    intvs = int(DELV_dic['time'][-1])
    thrp_list = []
    for i in range(intvs):
        time_end = time_start+thrp_intv
        mark = (DELV_dic['time']>=time_start) & (DELV_dic['time']<time_end)
        payloads_size = np.array(DELV_dic['size'])[mark]
        DELV_dic['throughput'].append(np.sum(payloads_size)*8/thrp_intv) # 单位: bps
        time_start += thrp_intv
    plot_throughput(range(intvs), DELV_dic['throughput'], thrp_intv)

    interval_t = DELV_dic['time'][-1] - DELV_dic['time'][0]
    total = 0
    for i in DELV_dic['throughput']:
        total = total + i
    log(f"ave rate: {total}/{interval_t}={total/interval_t}")
    log(f"{total/interval_t}")

shutil.copy('./log.log', f"/vagrant/tju_tcp/figure/{t}/log.log")
print(f"当前 log 文件复制到, /vagrant/tju_tcp/figure/{t}/log.log")
