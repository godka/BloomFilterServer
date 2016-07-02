# Bloom Filter Server的制作与搭建 #
**黄天驰，陈旭，孙留芳**

本小组最终选择了制作一个实用的**布隆过滤器服务器**来作为小组大作业。众所周知，布隆过滤器运用广泛，特别在爬虫领域，多爬虫为了避免所谓的“环路”问题，即出现多次访问一个url的问题，非常依赖布隆过滤器。故制作一个地位类似于redis的，轻量级的布隆过滤器服务器是很有必要的。以下将布隆过滤器服务器简称为**BFS**。

>布隆过滤器服务器基本要求如下：
>1. 跨平台，多个平台都可以快速部署BFS。
>2. 服务器支持c10k级别高并发查询和增加。
>3. 查询和增加时间效率最好为O(1)。
>4. 留下接口，可以进行简单的二次开发。

## 布隆过滤器 ##

布隆过滤器是由布隆在1970年提出的。它实际上是由一个很长的二进制向量和一系列随机映射函数组成，布隆过滤器可以用于检索一个元素是否在一个集合中。它的优点是空间效率和查询时间都远远超过一般的算法，缺点是有一定的误识别率（假正例False positives，即Bloom Filter报告某一元素存在于某集合中，但是实际上该元素并不在集合中）和删除困难，但是没有识别错误的情形（即假反例False negatives，如果某个元素确实没有在该集合中，那么Bloom Filter 是不会报告该元素存在于集合中的，所以不会漏报）。

**简单的说，布隆过滤器算法是以哈希表为基础的算法。过滤器通过运行k哈希函数来给一个内存区域标上记号来达到标记序列。这个序列可以是任何数据类型的数据。**

### 在BFS中使用的特殊的哈希表算法 ###

在标准布隆过滤器中，哈希函数构造方法被推荐为每次调用不同的哈希函数，如下所示：
```java
public class HashFunctions {
	public static int hash(byte[] bytes, int k) {
		switch (k) {
		case 0:
			return RSHash(bytes);
		case 1:
			return JSHash(bytes);
		case 2:
			return ELFHash(bytes);
		case 3:
			return BKDRHash(bytes);
		case 4:
			return APHash(bytes);
		case 5:
			return DJBHash(bytes);
		case 6:
			return SDBMHash(bytes);
		case 7:
			return PJWHash(bytes);
		}
		return 0;
	}
```
该方法有个巨大的问题，就是对于k来说，哈希函数总是有限的，以上实例中k的最大取值为7，最多只能进行8次哈希函数构造。**BFS根据标准布隆过滤器中有限个数哈希函数作为基础，提出了一个利用随机种子判断hash的方法。首先通过RSHash获取一个hash值，随后根据hash作为随机种子生成随机数，k个哈希函数即等价于k次随机数生成。**
详细算法如下所示：
```cs
        private int RSHash(String str)
        {
            int b = 378551;
            int a = 63689;
            int hash = 0;

            for (int i = 0; i < str.Length; i++)
            {
                hash = hash * a + str[i];
                a = a * b;
            }
            return (hash & 0x7FFFFFFF);
        }
        public void Add(string url)
        {
            var hash = RSHash(url);
            Random rand = new Random(hash);
            for (int i = 0; i < _k; i++)
            {
                _bytes[rand.Next(_bytes.Length)]++;
            }
        }
```
### BSF中对于m,n,k的取值探讨 ###
在标准布隆过滤器中，考虑出错率一般使用概率推导。

#### False positives 概率推导 ####

假设 Hash 函数以等概率条件选择并设置 Bit Array 中的某一位，m 是该位数组的大小，k 是 Hash 函数的个数，那么位数组中某一特定的位在进行元素插入时的 Hash 操作中没有被置位的概率是：

![][1]

那么在所有 k 次 Hash 操作后该位都没有被置 "1" 的概率是：

![][2]

如果我们插入了 n 个元素，那么某一位仍然为 "0" 的概率是：

![][3]

因而该位为 "1"的概率是：

![][4]

现在检测某一元素是否在该集合中。标明某个元素是否在集合中所需的 k 个位置都按照如上的方法设置为 "1"，但是该方法可能会使算法错误的认为某一原本不在集合中的元素却被检测为在该集合中（False Positives），该概率由以下公式确定：

![][5]

其实上述结果是在假定由每个 Hash 计算出需要设置的位（bit） 的位置是相互独立为前提计算出来的，不难看出，随着 m （位数组大小）的增加，假正例（False Positives）的概率会下降，同时随着插入元素个数 n 的增加，False Positives的概率又会上升，对于给定的m，n，如何选择Hash函数个数 k 由以下公式确定：

![][6]

此时False Positives的概率为：

![][7]

而对于给定的False Positives概率 p，如何选择最优的位数组大小 m 呢，

![][8]

上式表明，位数组的大小最好与插入元素的个数成线性关系，对于给定的 m，n，k，假正例概率最大为：

![][9]

#### 测试数据集的选取 ####
针对理论值，必须编写简单的测试程序以及选取测试集对理论值进行测试。我们经过一定的筛选，决定爬下朋友的网站[数据中心][10]。该网站全部通过站长手动添加，本质上是一个基本没有重复的url集合。我们基于组员之前编写的小型爬虫架构[Pink-Spider][11]制定了SimpleSpider用来专门挖取数据中心的url集合。**最终爬下来的集合如下所示,每一行都是一个不重复的url地址。**
```html
http://v.youku.com/v_show/id_XMTc1Nzg4NjEy.html?from=y1.2-1-89.3.4-1.1-1-1-3-0
http://v.youku.com/v_show/id_XMzMwMTAwOTg4.html?from=y1.2-1-89.3.1-1.1-1-1-0-0
http://v.youku.com/v_show/id_XODg1MDIzNjEy.html?from=y1.2-1-103.4.1-1.1-1-2-0-0
http://v.youku.com/v_show/id_XODk1MTU1OTYw.html?from=y1.2-1-89.4.17-1.1-1-2-16-0
http://v.youku.com/v_show/id_XNjQzMjgxNzQ0.html?from=y1.2-1-89.3.5-1.1-1-1-4-0
http://v.youku.com/v_show/id_XMTI5NDg2MDMy.html?from=y1.2-1-89.3.9-1.1-1-1-8-0
http://v.youku.com/v_show/id_XMTM4MDAyNzQyMA==.html?from=y1.2-1-103.3.8-1.1-1-1-7-0
http://v.youku.com/v_show/id_XMzUyMDM2MTg4.html?from=y1.2-1-95.4.3-1.1-1-2-2-0
http://v.youku.com/v_show/id_XNjA2Mzc0NjY4.html?from=y1.2-1-95.3.3-1.1-1-1-2-0
http://v.youku.com/v_show/id_XNDA0MDM5NjY4.html?from=y1.2-1-95.3.1-1.1-1-1-0-0
http://v.youku.com/v_show/id_XMjIwNTA4MTM2.html?from=y1.2-1-95.3.1-1.1-1-1-0-0
http://v.youku.com/v_show/id_XNDMxOTI4OTky.html?from=y1.2-1-95.4.6-1.1-1-2-5-0
http://v.youku.com/v_show/id_XMzI1MzQ0ODA4.html
http://tieba.baidu.com/p/4165848073
http://www.youxicheng.net/thyj/xinwen_6374.html
http://zhidao.baidu.com/question/326834156897041125.html
http://zhidao.baidu.com/question/486684828.html
```

#### BSF中对m,n,k取值 ####
根据理论值，为了确定m,n,k的值，我们特意编写了一个c#的软件进行模拟m,n,k在不同取值情况下对False Positive的影响。以下粗略展示了部分简单的测试代码。本代码简单的将数据集分成两半，一半写入布隆过滤器，一半判断是否在布隆过滤器内。**详细代码可以参考我们的[项目github][12]。**
```cs
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
namespace mythBloomFilter
{
    class Program
    {
        static void StartTest(string[] lines, int m)
        {
            CountBloomFilter bloomfilter = new CountBloomFilter(lines.Length / 2 * 10, m);
            for (var i = 0; i < lines.Length / 2; i++)
            {
                var line = lines[i];
                bloomfilter.Add(line);
            }
            int k = 0;
            for (var i = lines.Length / 2; i < lines.Length; i++)
            {
                var line = lines[i];
                if (bloomfilter.Contains(line))
                {
                    k++;
                }
            }
            Console.WriteLine("{0} : {1} / {2}", m, k, lines.Length / 2);
        }
        static void Main(string[] args)
        {
            var lines = File.ReadAllLines("out.txt");
            for (int i = 1; i < 20; i++)
                StartTest(lines, i);
            Console.WriteLine("done");
            Console.ReadKey();
        }
    }
}
```
经过数据集的测试后，最终结果如下，三个数字分别代表k的取值，False Positive的次数，False Positive测试集总大小：
```html
1 : 1497 / 15279
2 : 542 / 15279
3 : 264 / 15279
4 : 164 / 15279
5 : 141 / 15279
6 : 146 / 15279
7 : 134 / 15279
8 : 126 / 15279
9 : 154 / 15279
10 : 160 / 15279
11 : 167 / 15279
12 : 196 / 15279
13 : 256 / 15279
14 : 287 / 15279
15 : 330 / 15279
16 : 425 / 15279
17 : 483 / 15279
18 : 574 / 15279
19 : 681 / 15279
```
通过理论值可计算出k的最佳值大概在0.69 * 10 = 6.9~7的位置，但是实际最佳值在8，基本符合理论。**故在BSF中，m,n,k取值大致为10 * n, n ,7。**
## 布隆过滤器服务器 ##
在测试完布隆过滤器核心模块的准确性后，我们小组针对之前c#的测试代码用c语言进行了重构，并讨论服务器部件基本构成并选择各自的解决方法。
### 布隆过滤器核心 ###
布隆过滤器核心是参照之前c#的测试程序的c语言重构版本。该核心效率较高，每次查询和读取的时间复杂度为O(1)。核心代码如下所示。
```cpp
#include "BloomFliter.h"
#define MY_RAND(x) rand() % x
static int RSHash(const char* str, int len)
{
	int b = 378551;
	int a = 63689;
	int hash = 0;
	int i;
	for (i = 0; i < len; i++)
	{
		hash = hash * a + str[i];
		a = a * b;
	}
	return (hash & 0x7FFFFFFF);
}
void* BF_Create(int n){
	struct BloomFilterType* bf = (BloomFilterType*) malloc(sizeof(BloomFilterType));
	bf->buf = (char*) malloc(sizeof(char) * n * 10);
	bf->len = n * 10;
	memset(bf->buf, 0, bf->len);
	bf->k = 7;
	return bf;
}


void BF_Add(void* BF, const char* str, int len){
	int i;
	if (!BF)
		return;
	BloomFilterType* bft = (BloomFilterType*) BF;
	srand(RSHash(str, len));
	for (i = 0; i < bft->k; i++){
		bft->buf[MY_RAND(bft->len)] = 1;
	}
}

int BF_Contains(void* BF, const char* str, int len){
	int i;
	if (!BF)
		return NULL;
	BloomFilterType* bft = (BloomFilterType*) BF;
	srand(RSHash(str, len));
	for (i = 0; i < bft->k; i++){
		if (bft->buf[MY_RAND(bft->len)] == 0)
			return 0;
	}
	return 1;
}

int BF_Free(void* BF){
	if (BF){
		BloomFilterType* bft = (BloomFilterType*) BF;
		free(bft->buf);
		free(bft);
		bft = NULL;
	}
	BF = NULL;
}
```
在设计各个功能的时候我们特意将Type\*类型修改为void\*类型。**这样能够让更多语言在不需要知道核心数据结构的情况下调用，调用的时候只要传入核心数据结构指针即可。**代码简单，很容易理解，在此不再赘述。
布隆过滤器核心调用方法也很简单，拿最简单的添加元素来说，只需要初始化之后即可直接添加。在所有操作完成后必须将初始化的缓存区清空释放，否则会引起不必要的内存泄露。
```cpp
	void* bf_buf = BF_Create(1024 * 1024);			//创建n为1M的BF缓存区
	BF_Add(bf_buf, "Hello", 5);						//添加hello字符串
	BF_Free(bf_buf);							 	//释放缓存区
```
### 服务器报文选择 ###
在选择服务器报文上，我们小组进入了长考。对一款高性能布隆过滤器的服务器来说，报文由于效率的关系必须是不加密的明文，且该明文很容易被解析和生成。**最终我们选择了HTTP协议作为传输协议栈。**
#### 客户端请求 ####
客户端请求追求最大简化，大多数语言只需要有WebRequest的类即可快速开发完成。客户端发送的字符串为：
* 添加str
>http://ip:port/add=str

服务器返回ok表示添加成功。
* 查询str是否在BFS中
>http://ip:port/contain=str

服务器返回true表示str在BSF缓存池中，false表示str不在BSF缓存池中。

### 服务器架构选择 ###
服务器由于高并发与短连接特性，**我们谨慎选择了如今tcp服务器界非常火爆的开源库libevent进行实现**。libevent是一个基于事件触发的网络库，memcached底层也是使用libevent库。

**总体来说，libevent有下面一些特点和优势：**
* 事件驱动，高性能；
* 轻量级，专注于网络； 
* 跨平台，支持 Windows、Linux、Mac Os等； 
* 支持多种 I/O多路复用技术， epoll、poll、dev/poll、select 和kqueue 等； 
* 支持 I/O，定时器和信号等事件；

 

**libevent有下面几大部分组成：**

* 事件管理包括各种IO（socket）、定时器、信号等事件，也是libevent应用最广的模块；

* 缓存管理是指evbuffer功能；

* DNS是libevent提供的一个异步DNS查询功能；

* HTTP是libevent的一个轻量级http实现，包括服务器和客户端

BFS主要使用libevent中的HTTP模块实现。HTTP服务代码如下所示，**如需查看详细代码请查看我们的[项目github][13]。**
```cpp
#include "event2/listener.h"
#include "event2/bufferevent.h"
#include "event2/buffer.h"
#include "event2/event.h"
#include "event2/http.h"
#include "event2/http_struct.h"
#include "event2/keyvalq_struct.h"
#ifdef WIN32
#include <Winsock2.h>
#pragma comment(lib,"ws2_32")
#endif
#include <stdlib.h>
#include <stdio.h>
#include "BloomFliter.h"
void* bf_buf;
#define CMP(x) strcmp(header->key, x) == 0
#ifdef WIN32
int init_win_socket()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		return -1;
	}
	return 0;
}
#endif

void generic_handler(struct evhttp_request *req, void *arg)
{
	struct evbuffer *buf = evbuffer_new();
	if (!buf)
	{
		puts("failed to create response buffer \n");
		return;
	}
	const char*  reqbuf = evhttp_request_get_uri(req);
	//puts(reqbuf);
	struct evkeyvalq headers = { 0 };
	struct evkeyval *header;
	evhttp_parse_query_str(&reqbuf[1], &headers);

	for (header = headers.tqh_first; header;
		header = header->next.tqe_next) {
		if (CMP("add")){
			BF_Add(bf_buf, header->value, strlen(header->value));
			evbuffer_add(buf, "ok", 2);
			evhttp_send_reply(req, HTTP_OK, "ok", buf);
		}
		else if (CMP("contain")){
			if (BF_Contains(bf_buf, header->value, strlen(header->value))){
				evbuffer_add(buf, "true", 4);
				evhttp_send_reply(req, HTTP_OK, "ok", buf);
			}
			else{
				evbuffer_add(buf, "false", 5);
				evhttp_send_reply(req, HTTP_OK, "ok", buf);
			}
		}
		else{
			evhttp_send_reply(req, HTTP_BADREQUEST, "Error Command", buf);
		}
	}
	evbuffer_free(buf);
}

int main(int argc, char* argv [])
{
#ifdef WIN32
	init_win_socket();
#endif

	short          http_port = 6381;
	char          *http_addr = "0.0.0.0";

	struct event_base * base = event_base_new();

	struct evhttp * http_server = evhttp_new(base);
	if (!http_server){
		return -1;
	}

	int ret = evhttp_bind_socket(http_server, http_addr, http_port);
	if (ret != 0)
	{
		return -1;
	}

	evhttp_set_gencb(http_server, generic_handler, NULL);
	bf_buf = BF_Create(1024 * 1024);
	printf("Bloom Fliter Server start OK! \nServer Start at %d\n",http_port);

	event_base_dispatch(base);

	evhttp_free(http_server);
#ifdef WIN32
	WSACleanup();
#endif
	return 0;
}
```
在这里我们要解释一下该代码用到的某些技巧。在查询请求获取头部的时候我使用了该方法。
>evhttp_parse_query_str(&reqbuf[1], &headers);

取出指针偏移+1位置的作用在于屏蔽‘/’符号，并该操作并不存在内存泄露问题。随后定义了一个字典类型对该链表字典进行逐一取值。
>for (header = headers.tqh_first; header;header = header->next.tqe_next)

### 客户端调用 ###
客户端调用方式有很多，在此使用业内公认的curl进行测试。
* 测试添加
在bash中输入命令
>curl http://localhost:6381/add=hi
1. 成功
服务器返回ok
2. 失败
服务器返回bad request

```bash
$ curl http://localhost:6381/add=hi
ok
```
* 测试查询
在bash中输出命令
>curl http://localhost:6381/contain=hi
1. 成功
服务器返回 true
2. 失败
服务器返回false

```bash
$ curl http://localhost:6381/contain=hi
true
$ curl http://localhost:6381/contain=bye
false
```
### BenchMark并发测试 ###
在测试并发环节，我们使用了业内比较流行的Webbench进行简单的测试。
>testcommand:
>webbench -n 10000 -t 10 http://localhost:6381/add=hi
>webbench -n 10000 -t 10 http://localhost:6381/contain=hi

测试结果轻松并发10k
```bash
sty@deer:~/webbench-1.5$ ./webbench -c 10000 -t 3 http://localhost:6381/add=hi
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Benchmarking: GET http://localhost:6381/add=hi
10000 clients, running 3 sec.

Speed=1528020 pages/min, 1680734 bytes/sec.
Requests: 76401 susceed, 0 failed.
```
## 结语 ##
本次小组实验我们最终基于libevent制作了一个高性能高并发的跨平台布隆过滤器服务器，并已经加入小组成员的私人爬虫项目PinkSpider中。接下来我们将再在BFS上添加动态布隆过滤器的算法并争取精进，达到可以多次使用的水平。

  [1]: http://pic002.cnblogs.com/images/2012/274814/2012071316492245.png
  [2]: http://pic002.cnblogs.com/images/2012/274814/2012071316510534.png
  [3]: http://pic002.cnblogs.com/images/2012/274814/2012071316525966.png
  [4]: http://pic002.cnblogs.com/images/2012/274814/2012071316520528.png
  [5]: http://pic002.cnblogs.com/images/2012/274814/2012071317030828.png
  [6]: http://pic002.cnblogs.com/images/2012/274814/2012071317175487.png
  [7]: http://pic002.cnblogs.com/images/2012/274814/2012071317184158.png
  [8]: http://pic002.cnblogs.com/images/2012/274814/2012071317223869.png
  [9]: http://pic002.cnblogs.com/images/2012/274814/2012071317253418.png
  [10]: http://s.sjzx.xyz
  [11]: https://github.com/godka/pink-spider
  [12]: https://github.com/godka/bloomfilter
  [13]: https://github.com/godka/bloomfilterserver