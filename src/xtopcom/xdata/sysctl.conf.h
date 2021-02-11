#pragma once
#include <string>

const std::string sysctl_conf = R"(
# System default settings live in /usr/lib/sysctl.d/00-system.conf.
# To override those settings, enter new settings here, or in an /etc/sysctl.d/<name>.conf file
#
# For more information, see sysctl.conf(5) and sysctl.d(5).


#required Linux kernel 3.9.x+

    #not allow kernel handle ICMP packet,our raw socket may takeover all icmp
    net.ipv4.icmp_echo_ignore_all = 1
    
    #allow fragment for udp/tcp packet at vpn server
    net.ipv4.ip_no_pmtu_disc = 1
    
    #disable cache metrics so the initial conditions of the closed connections will not be saved to be used in near future connections
    #otherwise it may cause deny connection at some condition
    net.ipv4.tcp_no_metrics_save = 1 
    
    #Linux 2.4+ has strange behavior For example: The value for ssthresh for a given path is cached in the routing table. This means that if a connection has has a retransmition and reduces its window, then all connections to that host for the next 10 minutes will use a reduced window size, and not even try to increase its window. The only way to disable this behavior is to do the following before all new connections
    net.ipv4.route.flush = 1
    
	#net driver backlog for all socket, for very high speed network and data traffic;default is 1000
	net.core.netdev_max_backlog = 32768
	
	#the max ip packet of SoftIRQ can handle  in one cpu tick,default is 64
	net.core.dev_weight = 512
	
	#NAPI driver use this to determine total dev_weight of all interface(eth0,eth1...)
	net.core.netdev_budget = 4096
	
	#max queue size for sync requesting
	net.ipv4.tcp_max_syn_backlog = 16384
	
	#max queue size for waiting for accepted at listening queue
	net.core.somaxconn = 16384
	
	#enable tcp syn cookie that will apply when syn queue is full
	net.ipv4.tcp_syncookies = 0
	
	#enable time-wait recycle hat has negative effect for client from same ip(nat)
	net.ipv4.tcp_tw_recycle = 0
	
	#DON'T turn on when  tcp_tw_recycle is on
	net.ipv4.tcp_timestamps = 0
	
	#select ack
	net.ipv4.tcp_sack = 1
	net.ipv4.tcp_no_metrics_save = 1
	net.ipv4.tcp_window_scaling = 1
	
	#tcp_window_scaling and tcp_sack are turned on as default,so no need set again
	#net.ipv4.tcp_abort_on_overflow is 0 as default, which is good as well
	
	#max number of TIME_WAIT sockets, over this the socket will be released quickly
	net.ipv4.tcp_max_tw_buckets = 6000
	
	#config max-files,sysctl -w fs.file-max=2097152
	fs.file-max = 10485760

	#reserved 128m memory for system,recommend on for high memory instance
	vm.min_free_kbytes = 131072

	#set each udp socket min read memory(6M),sysctl -w net.ipv4.udp_rmem_min=6291456
	net.ipv4.udp_rmem_min = 6291456
	net.ipv4.udp_wmem_min = 6291456
	
	#set core(system,IP Layer) read memory for per socket (default:64, max: 6m) ,default for all kind of protocol
	net.core.rmem_default = 131072
	net.core.rmem_max = 8388688

	#set core(system,IP Layer) write memory for per socket(default:64, max: 6m),default for all kind of protocol
	net.core.wmem_default = 131072
	net.core.wmem_max = 8388688
	
	#set TCP Layer buffer(window) at min(4K),default(84K),max(4M)
	net.ipv4.tcp_rmem = 16384	131072	6291456
	net.ipv4.tcp_wmem = 16384	131072	4194304

	#raise higer for instance with >4G memory 
	#net.ipv4.tcp_mem = 133434	262144	533736
	
	#default it is 120 seconds
	net.ipv4.tcp_fin_timeout = 60

	#default 65536 that is too small for heavy server 
	net.netfilter.nf_conntrack_max = 524288
	#usally  nf_conntrack_buckets =  nf_conntrack_max / 4
	net.netfilter.nf_conntrack_buckets = 131072
 
 	#how long stay at timeout state, default it is 120 that is too long 
	net.netfilter.nf_conntrack_tcp_timeout_time_wait = 80
	
	# ip_conntrack/nf_conntrack has capacity limit ,set shorter timeout(default 600 seconds)
	# timeout for duration of no-packet at general level
	net.netfilter.nf_conntrack_generic_timeout = 80
	
	net.netfilter.nf_conntrack_icmp_timeout = 80
	
	# default is 432000 seconds(5 days,too long),for established status,6 hours are enough
	net.netfilter.nf_conntrack_tcp_timeout_established = 21600

	# decide how many local port used to connection out to remote
 	net.ipv4.ip_local_port_range = 10240 63000
 	
 	#enable pacing and fair-queue is good for TCP connection even for Cubic
 	#net.core.default_qdisc = fq
 	#enable BBR TCP congress 
	#net.ipv4.tcp_congestion_control = bbr
 	
 	kernel.core_uses_pid = 1
	kernel.core_pattern=core.%p.%e

)";

