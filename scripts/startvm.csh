qemu-system-x86_64 \
    -name vlm0,debug-threads=on \
    -machine ubuntu,accel=kvm \
    -cpu host,migratable=no \
    -enable-kvm -m 4096 \
    -object memory-backend-file,id=mem,size=4096M,mem-path=/dev/hugepages,share=on \
    -numa node,memdev=mem -mem-prealloc \
    -smp 2,sockets=1,cores=2,threads=1 -no-user-config \
    -nodefaults -rtc base=utc -boot strict=on \
    -device piix3-usb-uhci,id=usb,bus=pci.0,addr=0x1.0x2 \
    -drive file=/grid/vms/ub24.qcow2,if=none,id=drive-virtio-disk0,format=qcow2 \
    -device virtio-blk-pci,scsi=off,bus=pci.0,addr=0x5,drive=drive-virtio-disk0,id=virtio-disk0,bootindex=1 \
    -chardev pty,id=charserial0 -device isa-serial,chardev=charserial0,id=serial0 \
    -device virtio-balloon-pci,id=balloon0,bus=pci.0,addr=0x6 -msg timestamp=on -daemonize \
    -device virtio-vga \
    -vnc :1 \
    -chardev socket,id=mon0,path=/tmp/qemu-monitor.sock,server=on,wait=off \
    -mon chardev=mon0,mode=readline \
   -serial telnet:127.0.0.1:9900,server,nowait \
    -chardev qemu-vdagent,id=ch1,name=vdagent,clipboard=on \
    -device virtio-serial-pci \
    -device virtserialport,chardev=ch1,id=ch1,name=com.redhat.spice.0 \
    \
    -chardev socket,id=char1,path=/tmp/vhost-user0,reconnect=1 \
    -netdev type=vhost-user,id=hostnet1,queues=1,chardev=char1,vhostforce=on \
    -device virtio-net-pci,netdev=hostnet1,mq=on,vectors=4,id=net1,mac="00:60:2f:00:00:01",bus=pci.0,addr=0x7,csum=off,gso=off,guest_tso4=off,guest_tso6=off \
    ;
