# QEMU
we have already put some function hooks to help implementing two indirect branch handling mechanisms: a **shadow stack** and an **indirect branch target cache(IBTC)**.

## IBTC
Indirect branch target cache works similar to hardware cache, but it stores the address of the related code fragment stored in the code cache. Each time a cache lookup succeeds, the indirect branch can directly branch to the target address but not the emulation engine.

The following picture shows how IBTC works.
![image](https://github.com/a110605/qemu/blob/master/picture/1.png)
 
## Shadow Stack
 
## Quick Start
	# git clone https://github.com/a110605/qemu.git
	# cd qemu
	# ./configure --target-list=i386-linux-user
	The qemu executable is located at i386-linux-user/qemu-i386 after make
	# make

## Resources
The optimazation functions are inplemented in [qemu_dir]/optimization.c. 

For more details information, please refer the following documents

[VM_Homework 1 Report](https://www.dropbox.com/s/6kyfqhldz7fx2k0/VM_HW1_R03725019%E6%9D%8E%E5%A3%AB%E6%9A%84_R03725037%E6%9D%8E%E5%A5%95%E5%BE%B7_report.pdf?dl=0)