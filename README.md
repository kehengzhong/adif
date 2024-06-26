## adif - a C-Library of Commonly Used Data-structures and Algorithms

[README in English](https://github.com/kehengzhong/adif/blob/main/README-en.md)

*adif is a library of commonly-used data-structures and algorithms developed by C language. As the fundamental of application development, it facilitates the writing of high-performance programs, can greatly reduce the development cycle of software projects, enhance the efficiency of engineering development, and ensures the reliability and stability of software system.*

*用标准c语言开发的常用数据结构和算法基础库，作为应用程序开发接口基础库，为编写高性能程序提供便利，可极大地缩短软件项目的开发周期，提升工程开发效率，并确保软件系统运行的可靠性、稳定性。*


## 目录
* [adif是什么？](#adif是什么)
* [adif数据结构和算法库](#adif数据结构和算法库)
    * [基础数据结构](#基础数据结构)
    * [特殊数据结构](#特殊数据结构)
    * [常用的数据处理算法](#常用的数据处理算法)
    * [常用的字符串、字节流、字符集、日期时间等处理](#常用的字符串字节流字符集日期时间等处理)
    * [内存和内存池管理](#内存和内存池管理)
    * [配置文件、日志调试、文件访问、文件缓存、JSon、MIME等](#配置文件日志调试文件访问文件缓存jsonmime等)
    * [通信编程、文件锁、信号量、互斥锁、事件通知、共享内存等](#通信编程文件锁信号量互斥锁事件通知共享内存等)
    * [数据库访问管理](#数据库访问管理)
* [adif库开发的另外两个开源项目](#adif库开发的另外两个开源项目)
    * [ePump项目](#ePump项目)
    * [eJet Web服务器项目](#ejet-web服务器项目)
* [关于作者 老柯 (laoke)](#关于作者-老柯-laoke)

***

adif是什么？
------

adif 是用标准 c 语言开发的常用数据结构和算法基础库，是 Application Development Interface Fundamental 的缩写，作为应用程序开发接口基础库，为编写高性能程序提供便利，可极大地缩短软件项目的开发周期，提升工程开发效率，并确保软件系统运行的可靠性、稳定性。

众所周知，基于c语言开发的程序系统，其运行效率极其高效，跨平台跨OS操作系统兼容性极强，c语言是最接近操作系统内核和硬件指令集的高级编程语言，所以，开发OS系统、嵌入式系统、各类应用架构和基础平台、通信协议栈、大并发通信服务器、存储系统、流媒体系统、音视频编解码系统等等对性能有苛刻需求的系统，一般都采用c语言来开发。c语言从诞生至今几十年来，一直是各类商业级高性能业务开发的首选语言，历久弥新。正是c语言编程特点中极强的灵活性、高兼容性、运行高效性，无论高级语言如何发展、如何推陈出新、变化无穷，但使用c语言做高层应用、底层系统开发等前景依然广阔.

c语言开发门槛相对较高，编程人员除了对c语言语法熟悉外，还得掌握计算机组成原理、OS运行原理、数字逻辑、基础数据结构和算法等等，这就把很多爱好编程艺术的开发人员挡在门口。成熟稳定、实用性强的c语言基础库是c语言程序员越过编程门槛、享受高效编程乐趣的必备工具，而基于c语言的数据结构和算法库也陆续推出来不少，可能是脱离于实际应用开发、或者过于庞大，或者过于学院派，而很少能被广泛使用。

adif库是在开发各类应用系统的过程中积累下来的基础数据结构和算法库，经过大量的提炼优化和测试。初级编程人员可以使用adif来学习、体验c语言编程的乐趣，快速越过高性能编程的壁垒门槛；可以给高级c语言开发人员开发高性能程序提供成熟稳定的基础设施库，敏捷开发快速迭代，极大提升开发效率。

***

adif数据结构和算法库
------

adif 是标准c语言数据结构和算法库，数据结构和算法库实现主要包括基础数据结构、特殊数据结构、常用数据处理算法。这里包含了常用的字符串、字节流、字符集、日期时间等处理例程，还有内存管理和内存池的分配和释放管理等函数实现，此外还有，配置文件、日志调试、文件访问、文件缓存、JSon、MIME等功能模块。还提供了通信编程、文件锁、信号量、互斥锁、事件通知、共享内存等等接口函数。

### 基础数据结构

* **动态数组** - 数据插入、push、pop、删除、遍历、排序、搜索、二分查找、一致性哈希支持、自动内存分配.
* **栈** - 采用动态数组实现的先进后出FILO数据结构.
* **堆** - 是一种完全二叉树的数组对象，根节点最大的叫大顶堆、最大堆，可实现堆排序、优先队列、求最大或最小节点等功能
* **FIFO队列** - 采用动态线性数组实现的FIFO队列，添加和弹出无需移动内存.
* **链表** - 链表是一种不连续、非线性的数据存储和数据管理结构，支持插入、删除、遍历、搜索等操作
* **跳表** - 一种采用多级索引技术来实现二分查找的有序双向链表，支持插入、删除、遍历、搜索等操作
* **哈希表** - 一种对键值进行哈希运算并将结果映射到某个线性存储单元的数据结构，通过空间换时间取得数据读写的最高效率
* **红黑树** - 平衡二叉查找树，读写访问性能高于AVL树.


### 特殊数据结构

* **位数组** - 每一位bit的取值为0或1，其基本操作包括，基于位置标量来读写某一位的值，以及位左移右移、位与、位或、位异或等运>算操作，在高性能系统开发中特别常见
* **数据块数组** - 这是一种连续、线性的结构数组序列，采用数据块数组，可实现结构成员的插入、删除、快速定位、查找、动态内存扩充等操作
* **快速哈希表** - 普通哈希表对键值进行哈希运算后，映射到某个线性存储单元时，还需进行键值比较后才能访问数据。而快速哈希表的做法是多个哈希函数对键值进行多次哈希计算，主哈希值定位线性存储单元，辅助hash值进行匹配计算以确定是否访问单元.
* **布隆过滤器** - 判断一个数据是否存在于大数据集合中的最高效、最节省空间的概率型数据结构.
* **帧数据缓冲区** - 这是一种动态数据存储、读写、数据转换、访问管理等功能于一体的数据结构。它的功能包括管理数据缓冲区和管理其大小的动态变化，以及对缓冲区进行数据的追加、插入、删除、读取、搜索等操作，读写到文件、Socket I/O操作，数据编码格式操作等.
* **碎片数据块** - 编程中需要将不连续的、碎片化的来自内存块、文件、回调接口等地方的内容混合在一起，提供统一的、序列化的、连续的访问接口，进行遍历、读写、删除、添加、快速定位、模式匹配、检索、查找、数据转换等操作。chunk就是实现这些功能的一种数据结构。
* **字典树** - 又叫Trie树，单词查找树，常用于搜索引擎词频统计、分词处理，以及多模式匹配等


### 常用的数据处理算法

* **二分查找** - 基于动态数组、FIFO、跳表等线性数据结构上的折半查找算法，二分查找前提是基于排序的线性数据。将线性表的中间位置的关键字与查找关键字比较，如果两者相等，则查找成功，否则从中间位置将线性表拆分成前、后两个子表，继续取中间位置做比较运算，直到找到为止。
* **快速排序** - 将数据分割成两块，通过比较完成数据交换，对两块数据递归分割和排序，最终实现数据的有序
* **插入排序** - 通过二分法查找插入点，将新元素插入新位置
* **朴素匹配算法** - 逐个字节比较，不相等时回溯到第二个字节继续逐个比较，效率最低，时间复杂度为O(mn)
* **KMP匹配算法** - 比较失败后，不回溯到主串，直接跳转到后续位置，预先对模式串计算特征向量，时间复杂度为O(m+n)
* **Sunday匹配算法** - 对齐并比较主串和子串每个字符后，将主串中与子串位置对齐的下一个字符，与子串最右出现的字符通过移动主串的匹配位置来进行对齐，从而解决回溯问题。这个算法的平均时间复杂度为O(n)。
* **Boyer-Moore匹配算法** - 对模式串计算坏字符和好后缀数组，从右到左进行比较，当模式串比较失配后，按坏字符算法计算模式串需要右移的距离，按好后缀算法计算模式串需要右移的距离，取这两个右移距离的最大值，来移动模式串，并进行下一轮主串和模式串比较操作。最>好的时间复杂度为O(m/n)
* **Rabin-Karp哈希匹配算法** - 采用Rolling Hash算法，计算并比较子串和同长度主串的Rolling Hash值，不匹配就逐步挪动。平均时间复杂度为O(m+n)
* **Shift-And匹配算法** - 属于bitap算法的一种近似匹配算法，对子串进行预处理得出掩码集
* **Wu-Manber多模匹配算法** - 预先处理所有子串，取最短子串长度m，生成Shift、Hash、Prefix三张表，以m长度从右至左匹配主串，平均时间复杂度为O(Bn/m)
* **Aho-Corasick多模匹配算法** - 这个算法利用Trie树管理各子串，构建FailJump表，然后对主串进行匹配。其时间复杂度为O(m+n)


### 常用的字符串、字节流、字符集、日期时间等处理

* **字符串操作** - 合并、拷贝、比较、拆分、查找、输出、编码转换、裁剪、扫描、跳到、跳过、提取、转义等。
* **字节流操作** - 解析、跳转、转换和提取（各类整数、浮点数、字符串）、跳过和跳到（某些字符）、回退等
* **校验和** - 计算CRC校验和、Adler校验和
* **哈希函数** - Murmur 32位和64位hash函数、City Hash函数
* **编码转换** - 字节流和Base64编码相互转换、字节流和字符流转换、字符串escape/unescape处理、URL编码解码、HTML特殊字符编码解码
* **字符集处理** - 字符集转换、字符集自动识别
* **日期时间处理** - 获取当前系统时间、时间增减、时间比较、各种时间格式的字符串转为时间、时间转不同时间格式的字符串、定时器和定时任务调度器


### 内存和内存池管理

* **系统内存管理** - 对系统存储的分配、释放、重新分配、内存调测跟踪等。
* **内存池管理** - 同类数据结构的内存动态分配、回收、再分配、释放
* **基于大内存块的二次分配管理** - 预先从系统内存或共享内存分配一块大内存块，然后在这个内存块上，为应用程序提供所需的内存分配、释放、重新分配等操作.
* **对某数据结构分配一定数目的内存池** - 针对某个数据结构预先分配一定数量的内存单元，形成内存池。然后根据应用需求，基于该池来分配、回收、释放容纳该数据实例的内存单元。


### 配置文件、日志调试、文件访问、文件缓存、JSon、MIME等

* **配置文件** - 读取并解析标准Linux风格的配置文件、写入配置项、访问配置项的各类参数值等
* **日志调试** - 日志文件生成、日志记录写入、二进制字节流文本化输出等
* **文件访问** - 文件创建、打开、删除、读取、写入、定位，文件属性访问，文件拷贝、文件内存映射、文件零拷贝读写等
* **文件缓存** - 文件缓冲区创建、缓冲区读取、缓冲区写入、缓冲区线性访问、缓冲区查找、定位、缓冲区自动加载等
* **JSon** - JSon数据格式解析、JSon数据对象访问、JSon文件读写、JSon各类数据的访问等
* **MIME** - MIME类型列表管理、根据扩展名或MIMEID来获取MIME类型、根据MIME类型确定扩展名等


### 通信编程、文件锁、信号量、互斥锁、事件通知、共享内存等

* **Socket属性** - 设置阻塞和非阻塞、内核中的未读数据、Socket是否打开、读写是否ready、Socket地址解析和转换、DNS解析地址获取、IPv4和IPv6地址识别处理、Socket选项设置、NODELAY和NOPUSH选项设置等
* **TCP** - TCP服务器监听、TCP客户连接远程主机、TCP连接是否成功、TCP数据读、TCP写数据、TCP非阻塞读写、TCP零拷贝发送、TCP碎片内存写、TCP sendfile等
* **UDP** - UDP服务器监听、UDP数据报读写
* **SSL/TLS** - 基于TCP上的SSL/TLS安全连接创建、绑定TCP、解绑TCP、关闭、SSL加密传输读和写、文件加密发送等
* **Unix Socket** - Unix Socket server创建、接收usock请求、发起uscok请求等
* **本地IP地址** - 本地多IP地址读取
* **文件锁** - 基于fcntl的SETLKW/GETLKW来实现加锁、解锁，类似于Windows的有名互斥锁对象
* **信号量** - 多线程或多进程之间共享数据的互斥访问
* **事件通知** - 内核同步机制，可挂起当前线程或进程，直到特定的条件出现为止


### 数据库访问管理

* **MySQL数据库** - 数据库连接管理、Query查询、记录读取、Insert、Update、Delete、表的创建删除修改等。
* **SQLite数据库** - 数据库连接、表的select、insert、update、delete以及结果记录读取，表的创建删除修改
* **BerkeleyDB** - Key/Value数据库的创建、打开、get、put、mget、mput等

***

adif库开发的另外两个开源项目
------

### [ePump项目](https://github.com/kehengzhong/epump)

依赖于 adif 项目提供的基础数据结构和算法库，作者开发并开源了 ePump 项目。ePump 是一个基于I/O事件通知、非阻塞通信、多路复用、多线程等机制开发的事件驱动模型的 C 语言应用开发框架，利用该框架可以很容易地开发出高性能、大并发连接的服务器程序。ePump 框架负责管理和监控处于非阻塞模式的文件描述符和定时器，根据其状态变化产生相应的事件，并调度派发到相应线程的事件队列中，这些线程通过调用该事件关联的回调函数（Callback）来处理事件。ePump 框架封装和提供了各种通信和应用接口，并融合了当今流行的通信和线程架构模型，是一个轻量级、高性能的 event-driven 开发架构，利用 ePump，入门级程序员也能轻易地开发出商业级的高性能服务器系统。


### [eJet Web服务器项目](https://github.com/kehengzhong/ejet)

基于 adif 库和 ePump 框架开发的另外一个开源项目是 eJet Web 服务器，eJet Web 服务器项目是利用 adif 库和 ePump 框架开发的轻量级、高性能 Web 服务器，系统大量运用 Zero-Copy 技术，支持 HTTP/1.1、HTTPS 的全部功能，提供虚拟主机、URI rewrite>、Script脚本、变量、Cookie处理、TLS/SSL、自动Redirect、Cache本地存储、日志文件等功能，是静态文件访问、下载、以及PHP承载的理想平台，并对超大文件的上传发布提供高效支撑。此外，还支持 Proxy、 正向代理、反向代理、 TLS/SSL、FastCGI、 uWSGI、本地 Cache 存储管理、CDN节点服务等高级功能。eJet系统可以作为Web服务器承载 PHP 应用、Python 应用，同时利用缓存和 Proxy 功能，可以轻易地配置为 CDN 分发系统的重要分发节点。


***

关于作者 老柯 (laoke)
------

有大量Linux等系统上的应用平台和通信系统开发经历，是资深程序员、工程师，发邮件kehengzhong@hotmail.com可以找到作者，或者通过QQ号码[571527](http://wpa.qq.com/msgrd?V=1&Uin=571527&Site=github.com&Menu=yes)或微信号[beijingkehz](http://wx.qq.com/)给作者留言。

adif项目是作者三个关联开源项目的第一个项目，作为基础数据结构和算法库，是大量软件系统开发过程中积累提炼出来的，为其他项目提供底层开发支撑。本项目的很多代码早在上世纪九十年代末就已经开发完成，一直沿用至今，并在项目开发过程中不断优化和完善。

