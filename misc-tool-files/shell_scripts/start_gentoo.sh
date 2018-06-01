#!/bin/sh
exec qemu-system-x86_64 -enable-kvm \
        -cpu host -smp 2,sockets=1 \
        -drive file=/home/user/VirtualMachines/Machines/gentoo.img,if=virtio,cache=writethrough \
        -net nic,model=virtio -net user,hostname=gentoovm \
        -m 1024M \
        -name "Gentoo VM" \
        $@

## command to build testing kernel
## should modify /user/share/genkernel/gen_initramfs.sh first 
## in order to use as normal user (currently sudo)
#genkernel all --kerneldir=/home/user/Study/GitHub/linux --bootdir=/home/user/VirtualMachines/Machines/tmp_boot --logfile=/home/user/VirtualMachines/Machines/genkernel.log  --module-prefix=/home/user/VirtualMachines/Machines/tmp_boot --no-clean

## start debug kernel
#qemu-system-x86_64 -enable-kvm -cpu host -smp 2,sockets=1 -drive file=/home/user/VirtualMachines/Machines/gentoo.img,if=virtio,cache=writethrough -kernel /home/user/VirtualMachines/Machines/tmp_boot/kernel-genkernel-x86_64-4.0.0-rc3+ -initrd /home/user/VirtualMachines/Machines/tmp_boot/initramfs-genkernel-x86_64-4.0.0-rc3+ -append "root=/dev/vda3 init=/usr/lib/systemd/systemd console=uart8250,io,0x3f8 debug ignore_loglevel" -serial telnet:127.0.0.1:4444,server

#telnet 127.0.0.1 4444
