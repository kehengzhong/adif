## adif - a C-Library of Commonly Used Data-structures and Algorithms

[中文版 README](https://github.com/kehengzhong/adif/blob/main/README.md)

*adif is a library of commonly-used data-structures and algorithms developed by C language. As the fundamental of application development, it facilitates the writing of high-performance programs, can greatly reduce the development cycle of software projects, enhance the efficiency of engineering development, and ensures the reliability and stability of software system.*

*用标准c语言开发的常用数据结构和算法基础库，作为应用程序开发接口基础库，为编写高性能程序提供便利，可极大地缩短软件项目的开发周期，提升工程开发效率，并确保软件系统运行的可靠性、稳定性。*

## Table of Contents
* [What is adif?](#What-is-adif)
* [adif Data Structures and Algorithm Library](#adif-Data-Structures-and-Algorithm-Library)
    * [Basic Data Structures](#Basic-Data-Structures)
    * [Special Data Structures](#Special-Data-Structures)
    * [Common Data Processing Algorithms](#Common-Data-Processing-Algorithms)
    * [Common Routines Such as String, Byte-Stream, Charset, Date and Time](#Common-Routines-Such-as-String-Byte-Stream-Charset-Date-and-Time)
    * [Memory and Memory Pool Management](#Memory-and-Memory-Pool-Management)
    * [Configuration Files, Log Debugging, File Access, File Caching, JSON, MIME, etc.](#Configuration-Files-Log-Debugging-File-Access-File-Caching-JSON-MIME-etc)
    * [Communication Programming, File Locking, Semaphores, Mutexes, Event Notification, Shared Memory, etc.](#Communication-Programming-File-Locking-Semaphores-Mutexes-Event-Notification-Shared-Memory-etc)
    * [Database Access Management](#Database-Access-Management)
* [Two Other Open Source Projects Developed with the adif Library](#Two-Other-Open-Source-Projects-Developed-with-the-adif-Library)
    * [ePump Project](#ePump-Project)
    * [eJet Web Server Project](#eJet-Web-Server-Project)
* [About the Author LaoKe](#About-the-Author-LaoKe)


## What is adif?

ADIF is a fundamental library of common data structures and algorithms developed using ANSI-C language, which is an abbreviation for Application Development Interface Fundamental. Serving as a basic library for application development interfaces, it facilitates the writing of high-performance programs, can greatly reduce the development cycle of software projects, enhance engineering development efficiency, and ensure the reliability and stability of the software system.

Aa we know, programs developed with the C language have extremely high operational efficiency and strong cross-platform compatibility across various operating systems. C is a high-level programming language that is closest to the operating system kernel and hardware instruction set. Therefore, systems with demanding performance requirements such as operating systems, embedded systems, various application architectures and foundational platforms, communication protocol stacks, high-concurrency communication servers, storage systems, streaming media systems, audio and video encoding and decoding systems, etc., are generally developed using the C language. Since its inception decades ago, C has always been the language of choice for the development of various commercial high-performance applications, enduring and evergreen. It is precisely because of C's programming characteristics, which include great flexibility, high compatibility, and operational efficiency, that no matter how advanced languages evolve, innovate, or change infinitely, the prospects for using C language for high-level application and low-level system development remain broad.

The barrier to entry for C language development is relatively high. Programmers must not only be familiar with C language syntax but also need to master the principles of computer organization, operating system operation, digital logic, basic data structures, and algorithms, etc. This has kept many programming enthusiasts from entering the field. Mature, stable, and practical C language libraries are essential tools for C language programmers to overcome the programming barrier and enjoy the fun of efficient programming. Many data structure and algorithm libraries based on the C language have been introduced, but they are often detached from actual application development, too massive, too academic, and thus rarely widely used.

ADIF library is a collection of fundamental data structures and algorithms that have been accumulated during the development of various application systems. It has undergone extensive refinement, optimization, and testing. Junior programmers can use adif to learn and experience the joy of C language programming, quickly overcoming the barriers to high-performance programming. It also provides senior C language developers with a mature and stable infrastructure library for developing high-performance programs, enabling agile development and rapid iteration, thereby greatly enhancing development efficiency.


## adif Data Structures and Algorithm Library

adif is a standard C language data structure and algorithm library. The implementation of the data structure and algorithm library mainly includes basic data structures, special data structures, and commonly used data processing algorithms. It includes routines for processing common strings, byte streams, character sets, date and time, etc., as well as functions for memory management and memory pool allocation and release. In addition, there are functional modules for configuration files, log debugging, file access, file caching, JSON, MIME, etc. It also provides interface functions for communication programming, file locking, semaphores, mutexes, event notification, shared memory, and so on.


### Basic Data Structures

* **dynamic array** - provides data operation functions including inserting, pushing, pop, removing, traversing, sorting, searching, binary searching, conisitent hash, automatic allocation of memory
* **stack** - provides a FILO data structure based on dynamic array
* **heap** - provides an array object of a complete binary tree, and the root node with the max value is called the big-root heap or the max heap,  which can realize heap sorting, priority queue, maximum or minimum value and other functions.
* **FIFO queue** - provides a FIFO queue implemented by dynamic linear array, which can push and pop data without moving memory.
* **linked-list** - Linked list is a discontinuous and nonlinear data storage and data management structure, which supports operations such as insertion, deletion, traversal and search.
* **skip list** - Skip list is a kind of ordered double-linked list which uses multilevel index to realize binary search, and supports insertion and deletion traversal search and other operations.
* **hash table** - a kind of data structure that hashes key values and maps the results to a linear storage unit. It achieves the highest efficiency of data reading and writing by exchanging space for time.
* **red-black tree** - a balanced binary search tree, and its read-write access performance is higher than AVL tree.


### Special Data Structures

* **bit array** - The value of each bit is 0 or 1, and its basic operations include reading and writing the value of a bit based on the position scalar, and shifting the bit left and right, bit AND, bit OR, bit XOR etc., which is particularly common in the development of high-performance systems.
* **data-block array** - This is a continuous and linear structure array sequence. With data block array, operations such as inserting, deleting, quick locating, searching and dynamic memory expansion of structure members can be realized.
* **fast hash table** - When the key value is hashed by ordinary hash table and mapped to a linear storage unit, it is necessary to compare the key value before accessing the data. The practice of fast hash table is that multiple hash functions perform multiple hash calculations on key values, the main hash value locates the linear storage unit, and the auxiliary hash value performs matching calculation to determine whether to access the unit.
* **bloom filter** - This is the most efficient and space-saving probabilistic data structure to judge whether a data exists in a large data set.
* **data frame** - This is a data structure with functions of dynamic data storage, reading and writing, data conversion and access management. Its functions include managing the data buffer and the dynamic change of its size, as well as adding, inserting, deleting, reading and searching data to the buffer, reading and writing to files, Socket I/O operation, data encoding format operation and so on.
* **fragment data block** - In programming, discontinuous and fragmented contents from memory blocks, files, callback interfaces and other places need to be mixed together to provide a unified, serialized and continuous access interface for traversal, reading and writing, deletion, addition, quick positioning, pattern matching, retrieval, search, data conversion and other operations. Chunk is a data structure to realize these functions.
* **trie tree** - Also called dictionary tree, or word lookup tree. Commonly used in search engine word frequency statistics, word segmentation processing, and multi-pattern matching.


### Common Data Processing Algorithms

* **binary search** - Based on the binary search algorithm on linear data structures such as dynamic array, FIFO and skip table, the premise of binary search is based on sorted linear data. Compare the keyword in the middle of the linear table with the keyword. If they are equal, the search is successful. Otherwise, split the linear table into two sub-tables from the middle, and continue to compare the middle until it is found.
* **quick sort** - Divide the data into two pieces, complete the data exchange through comparison, recursively divide and sort the two pieces of data, and finally realize the order of the data.
* **insertion sort** - Insert the new element in the new position by finding the insertion point by dichotomizing.
* **naive matching algorithm** - Compare one byte at a time, and if it is not equal, go back to the second byte and continue to compare one by one, with the lowest efficiency and time complexity of O(mn).
* **KMP matching algorithm** - After the comparison fails, skip to the subsequent position without going back to the main string. This algorithm needs to calculate the feature vectors of pattern strings in advance. The time complexity is O(m+n).
* **Sunday matching algorithm** - After aligning and comparing each character of the main string and the substring, the next character aligned with the substring position in the main string is aligned with the rightmost character of the substring by moving the matching position of the main string, thus solving the backtracking problem. The average time complexity of this algorithm is O(n).
* **Boyer-Moore algorithm** - Calculate the array of bad characters and good suffixes for the pattern string, and compare it from right to left. When the pattern string is mismatched, calculate the distance that the pattern string needs to move to the right according to the bad character algorithm, and calculate the distance that the pattern string needs to move to the right according to the good suffix algorithm. Take the maximum of these two right shift distances to move the pattern string, and conduct the next round of comparison operation between the main string and the pattern string. The best time complexity is O(m/n).
* **Rabin-Karp hash matching algorithm** - Using the Rolling Hash algorithm, calculate and compare the Rolling Hash values of substring and main string with the same length. If it doesn't match, move it step by step. The average time complexity is O(m+n).
* **Shift-And matching algorithm** - An approximate matching algorithm belonging to bitap algorithm, which preprocesses the substring to get the mask set.
* **Wu-Manber multi-pattern matching algorithm** - Pre-process all substrings, take the shortest substring length m, and generate three tables of Shift, Hash and Prefix. Match the main string from right to left with the length of m, and the average time complexity is O(Bn/m).
* **Aho-Corasick multi-pattern matching algorithm** - This algorithm uses Trie tree to manage each substring, builds a FailJump table, and then matches the main string. Its time complexity is O(m+n).


### Common Routines Such as String, Byte-Stream, Charset, Date and Time

* **String operation** - Merge, copy, compare, split, search, output, conversion, crop, scan, jump, skip, extract, escape, etc.
* **byte-stream operation** - Parse, jump, transform and extract (all kinds of integers, floating-point numbers, strings), skip and jump (some characters), rewind, etc.
* **checksum** - Calculate CRC checksum and Adler checksum.
* **hash functions** - Murmur 32-bit and 64-bit hash functions, City Hash functions
* **coding conversion** - Byte stream and Base64 code conversion, byte stream and character stream conversion, string escape/unescape processing, URL coding and decoding, HTML special character coding and decoding.
* **charset processing** - Character set conversion and automatic recognition
* **processing of data and time** - Obtain the current system time, time increase and decrease, time comparison, strings with various time formats converted into or from time, timers and timing task schedulers.


### Memory and Memory Pool Management

* **system memory management** - allocation, release, reallocation of system storage, memory debugging and tracking, etc.
* **memory pool** - Dynamic allocation, recycle, reallocation and release of memory unit with the same data structure
* **memory allocation based on pre-allocated memory block** - Allocate a large memory block from system memory or shared memory in advance, and then provide the required memory allocation, release, reallocation and other operations for applications on this memory block.
* **memory pool for a data structure** - A certain number of memory cells are pre-allocated for a data structure to form a memory pool. Then, according to the application requirements, the memory unit containing the data instance is allocated, recycled and released based on the pool.


### Configuration Files, Log Debugging, File Access, File Caching, JSON, MIME, etc.

* **configuration files** - Read and parse standard Linux-style configuration files, write configuration items, and access various parameter values of configuration items.
* **log debugging** - Log file generation, log record writing, binary stream text output, etc.
* **file accessing** - File creation, opening, deletion, reading, writing, locating, file attribute access, file copy, file memory mapping, file zero-copy reading and writing, etc.
* **file caching** - File buffer creation, buffer reading, buffer writing, buffer linear access, buffer search, locating, buffer automatic loading, etc.
* **JSon** - JSon data format parsing, JSon data object access, JSon file reading and writing, JSon data access, etc.
* **MIME** - MIME type list management, obtaining MIME type according to extension name or MIMEID, determining extension name according to MIME type, etc.


### Communication Programming, File Locking, Semaphores, Mutexes, Event Notification, Shared Memory, etc.

* **Socket attributes** - Setting blocking and non-blocking, unread data in the kernel, whether the socket is open, read and write readiness, socket address resolution and transformation, DNS resolution for address acquisition, recognition and processing of IPv4 and IPv6 addresses, setting of socket options, NODELAY and NOPUSH options, etc.
* **TCP** - TCP server listening, TCP client connecting to a remote host, whether the TCP connection is successful, reading TCP data, writing TCP data, non-blocking reading and writing of TCP, zero-copy sending of TCP, fragmented memory writing of TCP, TCP sendfile, etc.
* **UDP** - UDP server listening, reading and writing of UDP datagrams.
* **SSL/TLS** - Creation of secure connections based on TCP with SSL/TLS, binding TCP, unbinding TCP, closing, encrypted reading and writing of SSL transmission, sending encrypted files, etc.
* **Unix Socket** - Creation of Unix Socket server, receiving usock requests, initiating usock requests, etc.
* **Local IP Address** - Configuration information reading of multiple local IP addresses
* **File locking** - Implementing locking and unlocking based on fcntl's SETLKW/GETLKW, similar to the named mutex lock object in Windows.
* **Semaphores** - Mutual exclusion for shared data access between multiple threads or processes.
* **Event notification** - A kernel synchronization mechanism that can suspend the current thread or process until specific conditions occur.


### Database Access Management

* **MySQL database** - Database connection management, query, record reading, Insert, Update, Delete, creation, deletion, modification of tables, etc.
* **SQLite database** - Database connection, table select, insert, update, delete, and result record reading, creation, deletion, modification of tables.
* **BerkeleyDB** - Creation, opening, get, put, mget, mput, etc., of Key/Value databases.


## Two Other Open Source Projects Developed with the adif Library

### [ePump Project](https://github.com/kehengzhong/epump)

Dependent on the basic data structures and algorithm libraries provided by the adif project, the author has developed and open-sourced the ePump project. ePump is an event-driven C language application development framework developed based on I/O event notification, non-blocking communication, multiplexing, multithreading, and other mechanisms. It is easy to develop high-performance, large-concurrent connection server programs using this framework. The ePump framework is responsible for managing and monitoring file descriptors and timers in non-blocking mode, generating corresponding events based on their state changes, and scheduling them to the event queues of corresponding threads. These threads handle events by calling the callback functions associated with the events. The ePump framework encapsulates and provides various communication and application interfaces, integrating today's popular communication and thread architecture models. It is a lightweight, high-performance event-driven development architecture. With ePump, even entry-level programmers can easily develop commercial-grade high-performance server systems.


### [eJet Web Server Project](https://github.com/kehengzhong/ejet)

Another open-source project developed based on the adif library and the ePump framework is the eJet Web Server. The eJet Web Server project is a lightweight, high-performance web server developed using the adif library and the ePump framework. The system extensively utilizes Zero-Copy technology and supports all features of HTTP/1.1 and HTTPS. It provides functionalities such as virtual hosting, URI rewriting, Script scripting, variables, Cookie handling, TLS/SSL, automatic Redirect, local Cache storage, and log files. It serves as an ideal platform for static file access, downloads, and hosting PHP applications, and provides efficient support for the upload and publication of ultra-large files. Additionally, it supports advanced features such as Proxy, forward and reverse proxy, TLS/SSL, FastCGI, uWSGI, local Cache storage management, and CDN node services. The eJet system can act as a web server to host PHP and Python applications, and with its caching and Proxy capabilities, it can be easily configured as an important distribution node of a CDN distribution system.


## About the Author LaoKe

With extensive experience in developing application platforms and communication systems on Linux and other systems, Lao Ke is a senior programmer and engineer. The author can be reached via email at kehengzhong@hotmail.com, or messages can be left for the author through the QQ number 571527 or the WeChat ID beijingkehz.

The adif project is the first of the author's three related open-source projects. As a foundational data structure and algorithm library, it has been accumulated and refined during the development process of numerous software systems, providing underlying development support for other projects. A lot of the code for this project was already developed in the late 1990s and has been in continuous use to this day, with ongoing optimization and improvement throughout the development process.

