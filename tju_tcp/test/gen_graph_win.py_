#encoding: utf-8
import matplotlib.pyplot as plt
import sys
import numpy as np

font_size = 15

def plot_win(time_list, win_list, win_type, type=[]):
    plt.plot(time_list, win_list, color='black')
    plt.xlabel('Time (s)', fontdict={'size':font_size})
    plt.ylabel('%s Window Size (segment)'%win_type, fontdict={'size':font_size})

    if win_type=='Congestion':
        map_color = {0: 'red', 1: 'green', 2:'blue', 3:'cyan'}
        map_label = {0: 'slow start', 1: 'congestion avoidance', 2:'fast retransmit', 3:  'timeout'}
        for item in range(4):
            idx = np.argwhere(np.array(type)==item).reshape(1,-1).tolist()[0]
            plt.scatter(time_list[idx], win_list[idx], c=map_color[item], label=map_label[item])
        plt.legend()

    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    plt.savefig('/vagrant/tju_tcp/test/%sWindowSize_VS_Time.png'%win_type, dpi=600)
    print("绘制成功, 图像位于/vagrant/tju_tcp/test/%sWindowSize_VS_Time.png"%win_type)
    plt.cla()

def plot_all(cwnd_list, rwnd_list, swnd_list):
    plt.plot(cwnd_list[0], cwnd_list[1], color='red', label='cwnd')
    plt.plot(rwnd_list[0], rwnd_list[1], color='green', label='rwnd')
    plt.plot(swnd_list[0], swnd_list[1], color='blue', label='swnd')
    plt.legend()                                                    
    plt.legend(loc=0, numpoints=1)
    plt.xlabel('Time (s)', fontdict={'size':font_size})
    plt.ylabel('Window Size (segment)', fontdict={'size':font_size})
    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    plt.savefig('/vagrant/tju_tcp/test/AllWindowSize_VS_Time.png', dpi=600)
    print("绘制成功, 图像位于/vagrant/tju_tcp/test/AllWindowSize_VS_Time.png")
    plt.cla()

def plot_rtt(time_list, SampleRTT, EstimatedRTT, DeviationRTT, TimeoutInterval):
    plt.plot(time_list, SampleRTT, color='red', label='SampleRTT')
    plt.plot(time_list, EstimatedRTT, color='green', label='EstimatedRTT')
    plt.plot(time_list, DeviationRTT, color='blue', label='DeviationRTT')
    plt.plot(time_list, TimeoutInterval, color='black', label='TimeoutInterval')
    plt.legend()
    plt.xlabel('Time (s)', fontdict={'size':font_size})
    plt.ylabel('Time (ms)', fontdict={'size':font_size})
    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    plt.savefig('/vagrant/tju_tcp/test/RTT.png', dpi=600)
    print("绘制成功, 图像位于/vagrant/tju_tcp/test/RTT_VS_Time.png")
    plt.cla()

def plot_throughput(time_list, throuput_list, thrp_intv):
    plt.plot(time_list, throuput_list, color='black')
    plt.xlabel('Time (s)', fontdict={'size':font_size})
    plt.ylabel('throuput (bps)', fontdict={'size':font_size})
    plt.ylim(ymin=0, ymax=max(throuput_list)*1.05)
    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    plt.savefig('/vagrant/tju_tcp/test/Throuput.png', dpi=600)
    print("绘制成功, 图像位于/vagrant/tju_tcp/test/Throuput.png [注: 每%.3fs计算一次瞬时吞吐率]"%thrp_intv)
    plt.cla()

FILE_TO_READ = '/vagrant/tju_tcp/test/client.event.trace'
if len(sys.argv)>=2:
	FILE_TO_READ = sys.argv[1]
print("正在使用 %s Trace文件绘图"%FILE_TO_READ)

SEND_dic = {'utctime':[], 'seq':[], 'ack':[], 'flag':[], 'length':[]}
RECV_dic = {'utctime':[], 'seq':[], 'ack':[], 'flag':[], 'length':[]}
CWND_dic = {'utctime':[], 'type':[], 'size':[]}
RWND_dic = {'utctime':[], 'size':[]}
SWND_dic = {'utctime':[], 'size':[]}
RTTS_dic = {'utctime':[], 'SampleRTT':[], 'EstimatedRTT':[], 'DeviationRTT':[], 'TimeoutInterval':[]}
DELV_dic = {'utctime':[], 'seq':[], 'size':[], 'throughput':[]}

start_time = 0
with open(FILE_TO_READ, 'r', encoding='utf-8') as f:
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
            SEND_dic['length'].append(int(info_list[3]))
        elif line_list[1] == 'RECV':
            RECV_dic['utctime'].append(int(line_list[0]))
            RECV_dic['seq'].append(int(info_list[0]))
            RECV_dic['ack'].append(int(info_list[1]))
            RECV_dic['flag'].append(info_list[2])
            RECV_dic['length'].append(int(info_list[3]))
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

# 间隔100个数据进行绘制
intv = 100
plot_win(CWND_dic['time'][::intv], np.array(CWND_dic['size'][::intv]), 'Congestion', CWND_dic['type'])
plot_win(RWND_dic['time'][::intv], RWND_dic['size'][::intv], 'Receive')
plot_win(SWND_dic['time'][::intv], SWND_dic['size'][::intv], 'Send')
plot_all([CWND_dic['time'][::intv], CWND_dic['size'][::intv]], [RWND_dic['time'][::intv], RWND_dic['size'][::intv]], [SWND_dic['time'][::intv], SWND_dic['size'][::intv]])

if len(RTTS_dic['utctime']): 
    plot_rtt(RTTS_dic['time'][::intv], RTTS_dic['SampleRTT'][::intv], RTTS_dic['EstimatedRTT'][::intv], RTTS_dic['DeviationRTT'][::intv], RTTS_dic['TimeoutInterval'][::intv])

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
  