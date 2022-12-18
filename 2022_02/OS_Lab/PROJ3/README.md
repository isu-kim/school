# Proj3 - MyFS
An implementation for simple file system for os lab fall semester 2022.
## Features
- File system implementation using "left child right sibling tree". 
- Basic "unix-like" commands:
    - `ls`: directory listing
    - `cat`: retrieve file content
    - `mkdir`: create empty directory
    - `touch`: create empty file
    - `rm`: delete file
    - `rmdir`: delete directory recursively
    - `cd`: change current working directory
    - `stat`: show stats of designated file
    - `vstat`: show stat of volue
    - `cwd`: show current working directory
    - `cp`: copy a file (CoW)
    - `mv`: move a file 
    - `rename`: rename a file
    - `xmas`: print a christmas tree

## Todo - Basic
 - [x] `mkdir`
 - [x] `touch test.txt`: File creation
 - [x] `echo >> test.txt`: File write
 - [x] `chmod`
 - [ ] `ls` other directory (optional)
 - [ ] `cat` other directory (optional)
 - [x] `rm` file
 - [x] `rmdir`

## Todo - Advanced
 - [ ] RAID 0 with strafing. -> Consider each `.img` file as a disk.
 - [ ] `mount` to directory. `mount disk0 /test/`.
 - [ ] disk file generator
 - [ ] scheduler 
