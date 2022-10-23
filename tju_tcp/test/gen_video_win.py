#encoding: utf-8
import numpy as np
import moviepy.editor as mpy
import matplotlib.pyplot as plt
import matplotlib.patches as pch
from moviepy.video.io.bindings import mplfig_to_npimage

"""
本程序用于绘制可靠传输的窗口滑动视频
使用的前提是 在test文件夹下, 且test文件夹中包含 client.event.trace 和 server.event.trace'
在test文件夹下使用 python3 gen_video_win.py 即可运行本程序, 并生成slidewin.mp4
不强制所有同学使用本程序, 感兴趣的同学使用即可

本程序包括三部分:日志数据读取、日志数据处理、视频生成
部分参数需要大家自行调整
代码如有问题, 欢迎在共享文档上给我们意见和建议: 【腾讯文档】gen_video_win.py修改意见https://docs.qq.com/doc/DQVh5VlhpTndSeHlv
"""

def read_trace(file, min_time=-1):
    SEND_dic = {'utctime':[], 'seq':[], 'ack':[], 'flag':[], 'length':[]}
    RECV_dic = {'utctime':[], 'seq':[], 'ack':[], 'flag':[], 'length':[]}
    CWND_dic = {'utctime':[], 'type':[], 'size':[]}
    RWND_dic = {'utctime':[], 'size':[]}
    SWND_dic = {'utctime':[], 'size':[]}
    RTTS_dic = {'utctime':[], 'SampleRTT':[], 'EstimatedRTT':[], 'DeviationRTT':[], 'TimeoutInterval':[]}
    DELV_dic = {'utctime':[], 'seq':[], 'size':[], 'throughput':[]}

    with open(file, 'r', encoding='utf-8') as f:
        for num, line in enumerate(f):
            if(line=='\n'): continue # 跳过空行
            if('SEND' not in line and 'RECV' not in line and 'CWND' not in line and 'RWND' not in line 
            and 'SWND' not in line and 'RTTS' not in line and 'DELV' not in line): continue # 跳过非事件行
            line = line.strip('\n'); line = line.replace('[', ''); line = line.replace(']', '')
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
                CWND_dic['size'].append(int(info_list[1]))
            elif line_list[1] == 'RWND':
                RWND_dic['utctime'].append(int(line_list[0]))
                RWND_dic['size'].append(int(info_list[0]))
            elif line_list[1] == 'SWND':
                SWND_dic['utctime'].append(int(line_list[0]))
                SWND_dic['size'].append(int(info_list[0]))
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
    
    if min_time==-1:
        for (utctime,flag_value) in zip(SEND_dic['utctime'],SEND_dic['flag']):
            if flag_value == '0':
                min_time = utctime
                break
        
    SEND_dic['time'] = [item - min_time for item in SEND_dic['utctime']]
    RECV_dic['time'] = [item - min_time for item in RECV_dic['utctime']]
    CWND_dic['time'] = [item - min_time for item in CWND_dic['utctime']]
    RWND_dic['time'] = [item - min_time for item in RWND_dic['utctime']]
    SWND_dic['time'] = [item - min_time for item in SWND_dic['utctime']]
    RTTS_dic['time'] = [item - min_time for item in RTTS_dic['utctime']]
    DELV_dic['time'] = [item - min_time for item in DELV_dic['utctime']]
    SEND_dic['time'] = np.divide(SEND_dic['time'], 1000000) # 单位: s
    RECV_dic['time'] = np.divide(RECV_dic['time'], 1000000)
    CWND_dic['time'] = np.divide(CWND_dic['time'], 1000000)
    RWND_dic['time'] = np.divide(RWND_dic['time'], 1000000)
    SWND_dic['time'] = np.divide(SWND_dic['time'], 1000000)
    RTTS_dic['time'] = np.divide(RTTS_dic['time'], 1000000)
    DELV_dic['time'] = np.divide(DELV_dic['time'], 1000000)
    
    print("read trace ok")
    return SEND_dic, RECV_dic, CWND_dic, RWND_dic, SWND_dic, RTTS_dic, DELV_dic, min_time

############################################ 日志数据读取 #########################################################
SEND_dic_client, RECV_dic_client, CWND_dic_client, RWND_dic_client, SWND_dic_client, RTTS_dic_client, DELV_dic_client, min_time_client = read_trace('client.event.trace')
SEND_dic_server, RECV_dic_server, CWND_dic_server, RWND_dic_server, SWND_dic_server, RTTS_dic_server, DELV_dic_server, min_time = read_trace('server.event.trace', min_time_client)

############################################ 日志数据处理 #########################################################
rwnd = 64512 # 接收窗口长度（byte） 【自行调整：填写自己完整接收窗口的大小】

# 字典l_x是 时间 与 项目x的大小值 之间的对应关系
l_base_swnd = {} # 发送窗口起始位置
l_base_rwnd = {} # 接收窗口起始位置
l_next_seq = {}  # next sequence 
l_swnd = {} # 发送窗口大小
l_rwnd = {} # 可用接收缓冲大小
l_rwnd[0] = rwnd
l_recv_list = {} # 窗口内 乱序到达的 数据的列表 [[data1_begin,data1_end], [data2_begin,data2_end], [data3_begin,data3_end],...]

################## CLIENT.TRACE ##################
# 根据三次握手的[SEND]事件 获取窗口的一些初始化参数
for (flag_value,seq_value) in zip(SEND_dic_client['flag'],SEND_dic_client['seq']):
    if flag_value == '8': # SYN
        base_swnd = seq_value + 1
        base_rwnd = seq_value + 1
        l_base_swnd[0] = base_swnd
        l_base_rwnd[0] = base_rwnd
        break

# 根据可靠数据传输的[SEND]事件 改变next_seq
next_seq = -1
for (seq_value,flag_value,len_value,time) in zip(SEND_dic_client['seq'],SEND_dic_client['flag'],SEND_dic_client['length'],SEND_dic_client['time']):
    if len_value > 0:
        l_next_seq[time] = max(seq_value + len_value, next_seq)
        next_seq = l_next_seq[time]

# 根据可靠数据传输的[SWND]事件 更新发送窗口大小swnd 
for (time,size) in zip(SWND_dic_client['time'],SWND_dic_client['size']):
    l_swnd[time] = size

# 根据可靠数据传输[RECV]事件 更新发送窗口起点值 
for (time,flag_value,ack_value) in zip(RECV_dic_client['time'],RECV_dic_client['flag'],RECV_dic_client['ack']):
    if (flag_value == '4') and (ack_value > base_swnd): # ACK
        l_base_swnd[time] = ack_value
        base_swnd = ack_value
                
# 根据可靠传输[RWND]事件 更新可用接收缓冲大小 
for (time,size) in zip(RWND_dic_client['time'],RWND_dic_client['size']):
    l_rwnd[time] = size

################## SERVER.TRACE ##################  
# 根据[DELV]事件 修改接收窗口起点值
for (time,seq_value,size_value) in zip(DELV_dic_server['time'],DELV_dic_server['seq'],DELV_dic_server['size']):
    if (seq_value + size_value) > base_rwnd:
        l_base_rwnd[time] = seq_value + size_value
        base_rwnd = seq_value + size_value
        
# 根据[RECV]事件 记录乱序到达的包
last_recv = []
l_recv_list[0] = [[l_base_rwnd[0],l_base_rwnd[0]]]
for time, seq_value, ack_value, flag_value, len_value in zip(RECV_dic_server['time'], RECV_dic_server['seq'], RECV_dic_server['ack'], RECV_dic_server['flag'], RECV_dic_server['length']):
    len_value -= 20
    if len_value>0:
        # 下一个 pkt 隔很久来的情况
        if (len(last_recv)>0):
            for tmp_ind in range(lastpkt_ind+1, len(l_base_rwnd.keys())):
                l_base_rwnd_tkey3 = list(l_base_rwnd.keys())[tmp_ind]
                if l_base_rwnd_tkey3 >= time:
                    break
                # lastpkt_t < tkey3
                tmp = np.array(last_recv)
                if(len(tmp.shape) > 1 ):
                    tmp = tmp[:,0]
                index_base = np.searchsorted(tmp, l_base_rwnd[l_base_rwnd_tkey3], side='right')
                l_recv_list[l_base_rwnd_tkey3] = last_recv[index_base:]
                last_recv = l_recv_list[l_base_rwnd_tkey3]
        
        this_item = [seq_value, seq_value+len_value]
        # 找到 base_rwnd 截断
        l_base_rwnd_ind = np.searchsorted(list(l_base_rwnd.keys()), time, side='right') # 找到 l_base_rwnd 中第一个发生在当前接收事件 后的 时间的下标
        l_base_rwnd_tkey = list(l_base_rwnd.keys())[max(0, l_base_rwnd_ind-1)] # 找到 l_base_rwnd 中当前接收事件 前的 最近的事件的时间
        l_base_rwnd_tkey2 = list(l_base_rwnd.keys())[min(l_base_rwnd_ind, len(list(l_base_rwnd.keys()))-1)]          # 找到 l_base_rwnd 中当前接收事件 后的 最近的事件的时间
        
        if this_item[1] > l_base_rwnd[l_base_rwnd_tkey]+rwnd: # 接收窗口后的数据 丢弃
            continue
        if this_item[0] < l_base_rwnd[l_base_rwnd_tkey]:  # 接收窗口前的数据 丢弃
            continue
        
        # time > tkey
        if len(last_recv) == 0: 
            l_recv_list[time] = [this_item]
            last_recv = l_recv_list[time]
        else:
            index_base = np.searchsorted(np.array(last_recv)[:,0], l_base_rwnd[l_base_rwnd_tkey], side='left')
            index_insert = np.searchsorted(np.array(last_recv)[:,0], seq_value, side='right')
            l_recv_list[time] = last_recv[index_base:index_insert] + [this_item] + last_recv[index_insert:]
            last_recv = l_recv_list[time]
        
        # time < tkey2
        index_base = np.searchsorted(np.array(last_recv)[:,0], l_base_rwnd[l_base_rwnd_tkey2], side='right')
        l_recv_list[l_base_rwnd_tkey2] = last_recv[index_base:]
        last_recv = l_recv_list[l_base_rwnd_tkey2]
        
        lastpkt_ind = l_base_rwnd_ind
             
# l_recv_list 按时间排序
l_recv_list = dict([(k,l_recv_list[k]) for k in sorted(l_recv_list.keys())] )
print("handle done")


############################################ 视频制作 #########################################################
time_extend = 1 # 减速倍数 1:原速；2:0.5倍速 【自行调整】
duration = 30 # 视频的总时长（单位：s） 【自行调整】
len_50m = 50*1024*1024 # 50M的长度，作为整个窗口滑动的长度

max_swnd = max(l_swnd.values()) # 最大发送窗口（byte）
expand_ratio = int(len_50m/(max_swnd*1.5)) 
# 局部缓冲区需要放大坐标系，这是放大的比例，即每个序列号都需要乘上这个比例才是画到局部缓冲区上的坐标； 
# 为了展示窗口滑动的效果，预留了一些空的位置，所以 max_swnd*1.5

time_keylist_base_swnd = list(l_base_swnd.keys())
time_keylist_next_seq  = list(l_next_seq.keys())
time_keylist_swnd      = list(l_swnd.keys())
time_keylist_base_rwnd = list(l_base_rwnd.keys())
time_keylist_recv_list = list(l_recv_list.keys())

def make_frame(t):
    # t为未缩放的视频时间
    # tkey_x 系列变量的用途是让 t时刻x变量的值 同tkey时刻x变量的值相同 
    tkey = time_keylist_base_swnd[0]
    for i in range(len(time_keylist_base_swnd)):
        if t < time_keylist_base_swnd[i] * time_extend:
            if i > 0:
                tkey = time_keylist_base_swnd[i-1]
            break
    
    tkey_nxseq = time_keylist_next_seq[0]
    for i in range(len(time_keylist_next_seq)):
        if t < time_keylist_next_seq[i] * time_extend:
            if i > 0:
                tkey_nxseq = time_keylist_next_seq[i-1]
            break
            
    tkey_swnd = time_keylist_swnd[0]
    for i in range(len(time_keylist_swnd)):
        if t < time_keylist_swnd[i] * time_extend:
            if i > 0:
                tkey_swnd = time_keylist_swnd[i-1]
            break
    
    tkey_base_rwnd = time_keylist_base_rwnd[0]
    for i in range(len(time_keylist_base_rwnd)):
        if t < time_keylist_base_rwnd[i] * time_extend:
            if i > 0:
                tkey_base_rwnd = time_keylist_base_rwnd[i-1]
            break
    
    tkey_recv_list = time_keylist_recv_list[0]
    for i in range(len(time_keylist_recv_list)):
        if t < time_keylist_recv_list[i] * time_extend:
            if i > 0:
                tkey_recv_list = time_keylist_recv_list[i-1]
            break
    
    fig_mpl, ax = plt.subplots(1,figsize=(20,5), facecolor='white')
    ax.set_xlim(-len_50m*0.05, len_50m*1.05)
    ax.set_ylim(-6,6)
    plt.text(len_50m*0.5, 5.5, "Send Window", weight="bold", color="black", size=15)
    plt.text(len_50m*0.5, -4.5, "Recv Window", weight="bold", color="black", size=15)
    plt.axis('off')
    
    ############################## 绘制两条完整缓冲区 (绘制成功后保持不变) ##################################
    # 绘制整条发送缓冲区
    whole_sbuf = pch.Rectangle(xy=(0, 0.75), width=len_50m, height=0.5, color='whitesmoke', linewidth=3)
    ax.add_patch(whole_sbuf)

    # 绘制发送窗口
    whole_swnd = pch.Rectangle(xy=(l_base_swnd[tkey], 0.75), width=l_swnd[tkey_swnd], height=0.5, color='slategrey', zorder=2, linewidth=3)
    ax.add_patch(whole_swnd)

    # 绘制整条接收缓冲区
    whole_rbuf = pch.Rectangle(xy=(0, -0.5), width=len_50m, height=0.5, color='whitesmoke', linewidth=3)
    ax.add_patch(whole_rbuf)

    # 绘制接收窗口
    whole_rwnd = pch.Rectangle(xy=(l_base_rwnd[tkey_base_rwnd], -0.5), width=rwnd, height=0.5, color='slategrey', zorder=2, linewidth=3)
    ax.add_patch(whole_rwnd)
    
    ############################## 绘制局部发送缓冲区 ##################################
    # 绘制局部发送缓冲区
    part_sbuf = pch.Rectangle(xy=(0, 2), width=len_50m, height=0.75, color='whitesmoke', linewidth=3)
    ax.add_patch(part_sbuf)

    # 计算base/nextseq/end的坐标
    part_base_loc    = l_base_swnd[tkey] * expand_ratio % len_50m # 取模表示让窗口在局部发送缓冲区循环展示
    part_nextseq_loc = l_next_seq[tkey_nxseq] * expand_ratio % len_50m
    part_end_loc     = (l_base_swnd[tkey] + l_swnd[tkey_swnd]) * expand_ratio % len_50m 
    
    # 绘制窗口的指示箭头 及 窗口放大的指示线
    ax.annotate("base:%d" %l_base_swnd[tkey],                       xy=(part_base_loc, 2.75),     xytext=(part_base_loc, 3.6),    arrowprops=dict(color='red', width=2, edgecolor='none'))
    ax.annotate("next seq:%d" %l_next_seq[tkey_nxseq],              xy=(part_nextseq_loc, 2.75),  xytext=(part_nextseq_loc, 4.1), arrowprops=dict(color='green', width=2, edgecolor='none'))
    ax.annotate("end:%d" %(l_base_swnd[tkey]+l_swnd[tkey_swnd]),    xy=(part_end_loc, 2.75),      xytext=(part_end_loc, 4.6),     arrowprops=dict(color='black', width=2, edgecolor='none'))
    ax.annotate("", xy=(l_base_swnd[tkey], 1.25),                   xytext=(part_base_loc, 1.95), arrowprops=dict(width=0.5,headwidth=0.02,color='black'))
    ax.annotate("", xy=(l_base_swnd[tkey]+l_swnd[tkey_swnd], 1.25), xytext=(part_end_loc, 1.95),  arrowprops=dict(width=0.5,headwidth=0.02,color='black'))

    # 绘制窗口色块
    # part_base_loc <= part_nextseq_loc <= part_end_loc
    if part_base_loc <= part_nextseq_loc and part_nextseq_loc <= part_end_loc: # send ack 块就不画了，因为循环画的时候不好区分send ack和unavailable的
        # 绘制 send noack 块
        part_send_noack = pch.Rectangle(xy=(part_base_loc, 2), width=part_nextseq_loc-part_base_loc, height=0.75, color='red', linewidth=3)
        ax.add_patch(part_send_noack)
        # 绘制 send available 块
        part_send_avail = pch.Rectangle(xy=(part_nextseq_loc, 2), width=part_end_loc-part_nextseq_loc, height=0.75, color='green', linewidth=3)
        ax.add_patch(part_send_avail)

    # part_end_loc <= part_base_loc <= part_nextseq_loc
    elif part_end_loc <= part_base_loc and part_base_loc <= part_nextseq_loc:
        # 绘制 send noack 块
        part_send_noack = pch.Rectangle(xy=(part_base_loc, 2), width=part_nextseq_loc-part_base_loc, height=0.75, color='red', linewidth=3)
        ax.add_patch(part_send_noack)
        # 绘制 send available 块（尾）
        part_send_avail = pch.Rectangle(xy=(part_nextseq_loc, 2), width=len_50m-part_nextseq_loc, height=0.75, color='green', linewidth=3)
        ax.add_patch(part_send_avail)
        # 绘制 send available 块（头）
        part_send_avail = pch.Rectangle(xy=(0, 2), width=part_end_loc, height=0.75, color='green', linewidth=3)
        ax.add_patch(part_send_avail)

    # part_nextseq_loc <= part_end_loc <= part_base_loc
    elif part_nextseq_loc <= part_end_loc and part_end_loc <= part_base_loc: 
        # 绘制 send noack 块（尾）
        part_send_noack = pch.Rectangle(xy=(part_base_loc, 2), width=len_50m-part_base_loc, height=0.75, color='red', linewidth=3)
        ax.add_patch(part_send_noack)
        # 绘制 send noack 块（头）
        part_send_noack = pch.Rectangle(xy=(0, 2), width=part_nextseq_loc, height=0.75, color='red', linewidth=3)
        ax.add_patch(part_send_noack)
        # 绘制 send available 块
        part_send_avail = pch.Rectangle(xy=(part_nextseq_loc, 2), width=part_end_loc-part_nextseq_loc, height=0.75, color='green', linewidth=3)
        ax.add_patch(part_send_avail)    
    
    ############################## 绘制局部接收缓冲区 ##################################
    # 绘制局部接收缓冲区 
    part_rbuf = pch.Rectangle(xy=(0, -2), width=len_50m, height=0.75, color='whitesmoke', linewidth=3)
    ax.add_patch(part_rbuf)

    # 计算base/end的坐标
    part_base_loc    = l_base_rwnd[tkey_base_rwnd] * expand_ratio % len_50m
    part_end_loc     = (l_base_rwnd[tkey_base_rwnd] + rwnd) * expand_ratio % len_50m

    # 绘制窗口的指示箭头
    ax.annotate("base:%d" %l_base_rwnd[tkey_base_rwnd],          xy=(part_base_loc, -2.1),      xytext=(part_base_loc, -3.4), arrowprops=dict(color='blue', width=2, edgecolor='none'))
    ax.annotate("end:%d" %(l_base_rwnd[tkey_base_rwnd]+rwnd),    xy=(part_end_loc, -2.1),       xytext=(part_end_loc, -3.7),  arrowprops=dict(color='black', width=2, edgecolor='none'))
    ax.annotate("", xy=(l_base_rwnd[tkey_base_rwnd], -0.5),      xytext=(part_base_loc, -1.25), arrowprops=dict(width=0.5,headwidth=0.02,color='black'))
    ax.annotate("", xy=(l_base_rwnd[tkey_base_rwnd]+rwnd, -0.5), xytext=(part_end_loc, -1.25),  arrowprops=dict(width=0.5,headwidth=0.02,color='black'))

    # 绘制局部接收窗口
    if part_base_loc <= part_end_loc: 
        part_rwnd = pch.Rectangle(xy=(part_base_loc, -2), width=part_end_loc-part_base_loc, height=0.75, color='lightblue', zorder=2, linewidth=3)
        ax.add_patch(part_rwnd)
    else:
        # 尾
        part_rwnd = pch.Rectangle(xy=(part_base_loc, -2), width=len_50m-part_base_loc, height=0.75, color='lightblue', zorder=2, linewidth=3)
        ax.add_patch(part_rwnd)
        # 头
        part_rwnd = pch.Rectangle(xy=(0, -2), width=part_end_loc, height=0.75, color='lightblue', zorder=2, linewidth=3)
        ax.add_patch(part_rwnd)
    
    # 绘制已接收部分数据
    for pkt in l_recv_list[tkey_recv_list]:
        pkt_s_loc = pkt[0] * expand_ratio % len_50m
        pkt_e_loc = pkt[1] * expand_ratio % len_50m
        
        if pkt_s_loc <= pkt_e_loc: 
            pkt_rwnd = pch.Rectangle(xy=(pkt_s_loc, -2), width=pkt_e_loc-pkt_s_loc, height=0.75, color='yellow', zorder=2, linewidth=3)
            ax.add_patch(pkt_rwnd)
        else:
            # 尾
            pkt_rwnd = pch.Rectangle(xy=(pkt_s_loc, -2), width=len_50m-pkt_s_loc, height=0.75, color='yellow', zorder=2, linewidth=3)
            ax.add_patch(pkt_rwnd)
            # 头
            pkt_rwnd = pch.Rectangle(xy=(0, -2), width=pkt_e_loc, height=0.75, color='yellow', zorder=2, linewidth=3)
            ax.add_patch(pkt_rwnd)
    plt.close('all')
    return mplfig_to_npimage(fig_mpl)

slidevideo = mpy.VideoClip(make_frame, duration=duration)
# slidevideo.write_gif('./slidewin.gif', fps=15)
slidevideo.write_videofile('./slidewin.mp4', fps=15)
