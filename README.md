
    使用多线程，因为event_base数支持多线程的，所以必须每个线程分配一个event_base,然而需要使用到一个函数event_base_set,
    为每个新的事件重新设置event_base,这样每个事件就会在该线程的反应堆中运行


    测试方法：
    

        测试并发性能：

        curl -v "http://127.0.0.1:8000/pub?cid=12&content=allen"

        webbench -c 15000 -t 30 "http://127.0.0.1:8100/sub?cid=12"      //不加序列号seq时，默认接受通道内所有消息
        或
        webbench -c 15000 -t 30 "http://127.0.0.1:8100/index.html"


        测试功能:

        (1) 测试/broadcast 广播功能

            向服务器广播消息:
                curl -v "http://127.0.0.1:8000/broadcast?content=comet"

            同时两个以上的客户去请求服务器：
                客户1: curl -v "http://127.0.0.1:8100/poll?cid=11"
                客户2: curl -v "http://127.0.0.1:8100/poll?cid=12"
                
            那么所有poll的客户会同时接收到服务器推送的消息

        （2）测试/pub 给指定通道推送消息的功能
            
            向服务器推送消息：
                curl -v "http://127.0.0.1:8000/pub?cid=12&content=allen"

            在cid=12的通道内获取消息：
                curl -v "http://127.0.0.1:8100/sub?cid=12"

            而cid=11的通道内是没有消息的：
                curl -v "http://127.0.0.1:8100/sub?cid=11"
                会得到没有消息的提示


          (3)测试通用回调函数
                curl -v "http://127.0.0.1:8100/index.html"



          (4) 增加一个处理时间分析功能，需要使用到boost库的时间组件
                增加了一个记录服务器运行时间的功能，operation::record_timer()函数记录服务器运行时间
                挂钟（日历）时间  用户CPU时间     系统CPU时间         总CPU时间占挂钟时间的百分比
                45.424396s wall, 1.640000s user + 21.180000s system = 22.820000s CPU (50.2%)



          (5) /clear 清空通道内消息功能
                curl -v "http://127.0.0.1:8000/clear?cid=12"
                清空通道12内的所有消息


    7个主要功能：

    (1)        pool:保持HTTP长连接,直到超时或者接收到消息后断开连接
    (2)        sub:连接后接收到消息就断开连接，如果没有消息断开连接
    (3)        read:读取指定文件的数据
    (4)        clear:清空指定通道内的消息
    (5)        pub:管理员推送消息到服务器，然后由服务器转发给用户
    (6)        broadcast:向所有通道广播消息
    (7)        通用回调函数：和/read作用是等价的


        pool: 如果通道内有消息，那么接收到消息之后就会即刻断开连接，如果没有消息就会保持HTTP连接，直到通道超时或用户超时 

        sub: 如果通道内有消息，接受消息后即刻断开连接，如果没有消息，接收指示无消息的报文，即刻断开连接，就是无论有无消息都即刻断开连接

        pub: 推送者向服务器推送消息，然后服务器将消息转发给用户，每个通道中有对应的消息队列，每一条消息都会推送到通道中的消息队列中，
            用户订阅消息就是从消息队列中读取消息


        主线程：
            主线程注册了三个事件：  
                (1) 中断信号处理事件
                (2) 终止信号处理事件
                (3) 定时器事件，定时器事件是每隔一定时间去检查是否有通道超时或用户超时，
                    如果是通道超时，那么断开该通道内所有用户的连接
                    如果是用户超时，那么断开该用户的连接

        服务器根据配置文件创建指定数量的线程
        每个子线程都注册了四个回调函数：
                pub_handler
                broadcast_handler : 这是广播消息给所有通道
                clear_handler
                sub_handler
                read_handler
                poll_handler
                generic_handler : 这是通用请求回调函数，默认将README.md文件发送给用户


    统计行数方法：

        不统计注释行（所有注释行跟在代码后，而不是另起一行）,所以这种方法统计的是纯代码行数
        忽略所有空白行：
            find . -name "*.cpp" | xargs cat | grep -v "^$" | wc -l
            find . -name "*.h" | xargs cat | grep -v "^$" | wc -l


        不忽略空白行：
            find . -name "*.cpp" | xargs cat | wc -l
            find . -name "*.h" | xargs cat | wc -l



    此版本是加入了连接数据库功能:

        将所有消息存放在数据库cometserver中，在cometserver中有一个表message(channel,seq,time,content)

        create table message(
            channel int not null,
            seq int not null,
            time date not null,
            content varchar(1024),
            PRIMARY KEY ('seq')
        )ENGINE=MyISAM DEFAULT CHARSET=utf8;

        客户的每次请求都从数据库中去查找

        服务器的每次推送都是向数据库写入



    设置主机访问cometServer服务器，需要在VMware中设置虚拟网络编辑中设置端口映射


    对于服务器的优化：  
                （1）优化处理客户请求数据的代码
                （2）优化加锁和解锁的环节，服务器在运行过程中有频繁的加锁个解锁行为


    重新编写处理错误代码方式：
                (1) 因为服务器是多线程环境，每个线程解析对应的http协议出错时，不应该影响到其他的线程，所以在所在线程不应该退出整个程序；
                (2) 将所有的解析错误处理记录级别设置为DEBUG级别，可用于调试作用；
                (3) 将程序的异常终止记录设置为ERROR级别；
                (4) 将所有的详细信息显示级别设置为TRACE；
                (5) 对于上锁和解锁失败，需要记录为致命错误，因为这会影响到程序的执行,级别为FATAL;








