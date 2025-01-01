//
// @file : ui.h
// @author : Isu Kim @ dev.gooday2die@gmail.com | Github @ https://github.com/gooday2die
// @brief : A file that defines all ui related functions.
//

#ifndef MYFS_UI_H
#define MYFS_UI_H
#pragma once

#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include "diskutil.h"
#include "common.h"

void main_loop();
void sig_handler(int);
void print_title();
void print_christmas();
void print_colored_christmas_tree();

int ls(char*, struct entry_t*);
int cat(char*, struct entry_t*);
int chmod(char*, struct entry_t*);
int stat(char*, struct entry_t*);
int touch(char*, struct entry_t*);
int mkdir(char*, struct entry_t*);
int rm(char*, struct entry_t*);
int rmdir_(char*, struct entry_t*); // rmdir is already defined in unistd.h :(
int vstat(char*);
int write_(char*, struct entry_t*); // write is already defined in unistd.h :(
int append(char*, struct entry_t*);
int cd(char*, struct entry_t*, struct entry_t**);
int cp(char*, struct entry_t*);
int rename_(char*, struct entry_t*); // rename is already defined in stdio.h :(
int mv(char*, struct entry_t*);

#endif //MYFS_UI_H
