#!/usr/bin/env python
# coding=utf-8

import sys, os

for i in range(1,len(sys.argv)):

# == (KERNEL_VERSION
  fr = open(sys.argv[i], "r")
  
  txt = fr.readlines()
  fr.close()
  
  os.remove(sys.argv[i])
  
  fr = open(sys.argv[i], "w+")
  del_flag = 0
  for line in txt:
    if "== (KERNEL_VERSION" in line:
        del_flag = 1

    if del_flag == 0:
      fr.write(line)
  
    if "#endif" in line:
        if del_flag == 1:
            del_flag = 0
 
  fr.close()

# != (KERNEL_VERSION
  fr = open(sys.argv[i], "r")
  
  txt = fr.readlines()
  fr.close()
  
  os.remove(sys.argv[i])
  
  fr = open(sys.argv[i], "w+")
  del_flag = 0
  for line in txt:
    if del_flag == 1:
      if "#endif" in line:
        del_flag = 0
        continue

    if "!= (KERNEL_VERSION" in line:
      del_flag = 1
    else:
      fr.write(line)

  fr.close()

# libgcc_ksyms.o
  fr = open(sys.argv[i], "r")
  
  txt = fr.readlines()
  fr.close()
  
  os.remove(sys.argv[i])
  
  fr = open(sys.argv[i], "w+")
  for line in txt:
    if "+= libgcc_ksyms.o" not in line:
      fr.write(line)

  fr.close()
