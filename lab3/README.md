# LAB3
Advance: 
https://blog.gtwang.org/linux/auto-execute-linux-scripts-during-boot-login-logout/amp/?fbclid=IwAR10tSF2LY1HqqzUZ66JvJUJ1uJpR56qNyqJzrOGaUWqyxVEQC8IZmGJiTo
https://felix-lin.com/linux/debianubuntu-%E6%96%B0%E5%A2%9E%E9%96%8B%E6%A9%9F%E8%87%AA%E5%8B%95%E5%9F%B7%E8%A1%8C%E7%A8%8B%E5%BC%8F/
https://www.readfog.com/a/1642962230824767488

* auto execute script after login
vim /home/embedsky/myscript.sh
```
LD_LIBRARY_PATH=/run/media/mmcblk1p1 /run/media/mmcblk1p1/demo
```
chmod +x /home/embedsky/myscript.sh
vim ~/.profile
```
# add 
/home/embedsky/myscript.sh
```

* auto execute script before login
vim /etc/init.d/mystart
chmod 755 /etc/init.d/mystart
update-rc.d mystart defaults

* remove

```update-rc.d -f mystart remove```

* runlevel

0   : Halt.
1   : Single user mode, no internet, no root.
2   : Multi user mode, no internet.
3   : Multi user mode, default startup.
4   : User customize.
5   : Multi user mode with GUI.
6   : Reboot.


* Using systemd (embedded system can run?)
vim /lib/systemd/system/restart.service
```
[Unit]
Description=restart
After=default.target

[Service]
ExecStart=/root/script/restart.sh

[Install]
WantedBy=default.target
```
mkdir /root/script
vim /root/script/restart.sh
```
#!/bin/bash
LD_LIBRARY_PATH=/run/media/mmcblk1p1 /run/media/mmcblk1p1/demo
exit 0
```
chmod +x /root/script/restart.sh
systemctl daemon-reload
systemctl enable restart.service
systemctl status restart.service