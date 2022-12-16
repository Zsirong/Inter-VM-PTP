# 在运行之前需要修改虚拟机的xml配置文件
虚拟机的配置文件存放路径为/etc/libvirt/qemu.

添加代码段
```
<controller type="virtio-serial" index="0" ports="16">
  <address type="pci" domain="0x0000" bus="0x04" slot="0x00" function="0x0"/>
</controller>
...
<channel type='unix'>
 <source mode='bind' path='/var/lib/libvirt/qemu/vm-ubuntu.ctl'/>
 <target type='virtio' address='virtio-serial' port='2'/>
</channel>
<channel type='unix'>
 <source mode='bind' path='/var/lib/libvirt/qemu/vm.data'/>
 <target type='virtio' address='virtio-serial' port='1'/>
</channel>
```

运行完后在主机中运行如下命令：
```
# virsh destroy vm2（关闭正在启动的虚拟机）
# vi /etc/libvirt/qemu/vm2.xml （修改xml配置文件）
# virsh define /etc/libvirt/qemu/vm2.xml （从 XML 文件定义（但不启动）域）
# virsh start vm2（开启虚拟机）
```

然后重新启动虚拟机  
修改client.c和server.c源文件中相关文件路径 CONTROL_PORT和 PORT_ubuntu  
在虚拟机中运行client.c，在主机中运行server.c
