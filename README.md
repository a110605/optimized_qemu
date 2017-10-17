# QEMU
In this project, we have implemented two indirect branch handling mechanisms: a **shadow stack** and an **indirect branch target cache(IBTC)** in QEMU full-system emulator to improve indirect branch performance.

## Indirect Branch Target Cache 
Indirect branch target cache works similar to hardware cache, but it stores the address of the related code fragment stored in the **code cache**.

Each time a cache lookup succeeds, the indirect branch can directly branch to the target address but not the emulation engine.

The following picture shows how IBTC works.

 <img src="https://i.imgur.com/6GdcjwB.png" width="70%" height="70%">

## Shadow Stack
A shadow stack is used to accelerate searching branch targets when current instruction is a **return instruction**.

The address of the translation block corresponding to next instruction of the function call is pushed on the shadow stack while the function call is executed.

When the callee returns, the top of the shadow stack is popped, and the popped address is the translation block of the return address.

The following picture shows how shadow stack works.

 <img src="https://i.imgur.com/3knYnRS.png" width="60%" height="60%">


## Performance Experiments
<img src="https://i.imgur.com/JEbcKOL.png" width="60%" height="60%">

<img src="https://i.imgur.com/EjzQlwj.png" width="60%" height="60%">

<img src="https://i.imgur.com/TbjCBxk.png" width="60%" height="60%">


## Quick Start
	# git clone https://github.com/a110605/qemu.git
	# cd qemu
	# ./configure --target-list=i386-linux-user
	The qemu executable is located at i386-linux-user/qemu-i386 after make
	# make
	

## Resources
The optimization functions are inplemented in [qemu_dir]/optimization.c. 

For more details information, please refer the following documents

- [VM_Homework 1 Report](https://www.dropbox.com/s/6kyfqhldz7fx2k0/VM_HW1_R03725019%E6%9D%8E%E5%A3%AB%E6%9A%84_R03725037%E6%9D%8E%E5%A5%95%E5%BE%B7_report.pdf?dl=0)
