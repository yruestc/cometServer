    Linux使用dd命令创建特定大小文件


    dd if=/dev/zero of=./index_1k.html bs=1k count=1

    dd if=/dev/zero of=./index_10k.html bs=10k count=1

    dd if=/dev/zero of=./index_100k.html bs=100k count=1


    创建三个文件，大小依次为1k,10k,100k
