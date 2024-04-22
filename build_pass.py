#!/usr/bin/env python3
import sys
import subprocess

def do_cmd(command, dir):
    p = subprocess.Popen(command, cwd=dir)
    p.wait()

clean_build = True
do_user_input_loop = True

if (len(sys.argv) > 1):
    cla = sys.argv[1]
    if (cla[1] == 'i' or cla[1] == 'c'):
        clean_build = cla[1] == 'c'
        do_user_input_loop = False
    

if do_user_input_loop:
    good_input_received = False
    while not good_input_received:
        print("c - clean build")
        print("i - intermediate build")

        user_in = input(">> ")

        if user_in == 'c' or user_in == 'i':
            clean_build = user_in == 'c'
            good_input_received = True

if clean_build:
    do_cmd(['rm', '-rf', 'build'], "./freshAttempt")
    do_cmd(['mkdir', 'build'], "./freshAttempt")
    do_cmd(['cmake', '..'], "./freshAttempt/build")

do_cmd(['make'], "./freshAttempt/build")