
## C10M(并发C10兆)，即: 千万并发

[背景] 2次拷贝，在千万并发时，存在过大的开销

1. 客户端的数据，先从网卡，copy到 <u>内核协议栈(TCP/IP协议栈,bsd)</u>
2. 从内核协议栈，copy到应用程序

把内核的协议栈，做到应用程序，目的是为了减少一次拷贝，即：从网卡直接拷贝到应用程序，中间不经过先拷贝到内核。

- 解决方案

  > **netmap 实现用户态协议栈**：①netmap会接管网卡eth0上的数据，直接将网卡上的数据mmap到内存中；②数据就不会经过内核了，应用层就可以直接从内存中读取数据
  >
  > 用户态协议栈，绕过内核


## netmap install
```
$ git clone https://github.com/wangbojing/netmap.git

$ ./configure

$ make 

$ sudo make install
```
## netmap install complete.

#### Troubleshooting

### 1. problem : configure --> /bin/sh^M. 

	you should run . 
```
$ dos2unix configure

$ dos2unix ./LINUX/configure
```
### 2. problem : cannot stat 'bridge': No such or directory

```
$ make clean

$ cd build-apps/bridge

$ gcc -O2 -pipe -Werror -Wall -Wunused-function -I ../../sys -I ../../apps/include -Wextra    ../../apps/bridge/bridge.c  -lpthread -lrt    -o bridge

$ sudo make && make install
```

## NtyTcp
netmap, dpdk, pf_ring, Tcp Stack for Userspace 

compile:
```
$ sudo apt-get install libhugetlbfs-dev

$ make
```

update NtyTcp/include/nty_config.h 
```

#define NTY_SELF_IP		"192.168.0.106" 	//your ip

#define NTY_SELF_IP_HEX	0x6A00A8C0 			//your ip hex.

#define NTY_SELF_MAC	"00:0c:29:58:6f:f4" //your mac
```

block server run:
```
$ ./bin/nty_example_block_server
```
epoll server run:
```
$ ./bin/nty_example_epoll_rb_server
```


