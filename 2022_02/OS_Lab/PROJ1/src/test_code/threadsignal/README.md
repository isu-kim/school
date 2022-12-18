# Usage
## Example Output
```
[+] PID : 631248
[+] Creating threads
[+] Joining
[P1] Created
[P2] Created
[P3] Created
```
## Example command
```
kill -10 631248
```
> For sending SIGUSR1


```
kill -12 631248
```
> For sending SIGUSR2

## Example Output
```
[P3] Got signal User defined signal 2
[P1] Got signal User defined signal 1
```
