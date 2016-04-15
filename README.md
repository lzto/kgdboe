#KGDBOE kernel debugging over ethernet

source code from
 http://sysprogs.com/VisualKernel/kgdboe/tutorial/

fixed to compile with kernel 4.5+

* Install
```
 make -C /lib/modules/$(uname -r)/build M=$(pwd)
 sudo insmod kgdboe.ko
```
* Configuration parameters
```
 insmod kgdboe.ko param1=val1 param2=val2 ...
```
* on host
```
 gdb> target remote udp:ip:port
```
