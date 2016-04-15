* Install

 make -C /lib/modules/$(uname -r)/build M=$(pwd)
 sudo insmod kgdboe.ko

Configuration parameters

 insmod kgdboe.ko param1=val1 param2=val2 ...


