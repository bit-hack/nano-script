#! /usr/bin/python

from __future__ import print_function
import os
import subprocess
import sys


is_linux = (sys.platform == "linux" or sys.platform == "linux2")


if is_linux:
    DRIVER = '../build/nano_driver'
    COMP = "../build/nano_comp"
else:
    DRIVER = '../build/Debug/nano_driver.exe'
    COMP = "../build/Debug/nano_comp.exe"


tried = set()
passed = set()


def get_expected(path):
    with open(path, "r") as fd:
        line = fd.readline()
    if line.startswith("#expect"):
        return line[8:].strip()
    return "--UNKNOWN--"


def try_comp(base, path):
    print('{0}'.format(path))
    tried.add(path)
    try:
        proc = subprocess.Popen(
            [COMP, path],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
        out, err = proc.communicate()
        ret = proc.returncode
    except OSError:
        print('failed to execute {0}'.format(path))


def do_comp(base, path):
    print('{0}'.format(path))
    tried.add(path)
    try:
        proc = subprocess.Popen(
            [COMP, path],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)

        out, err = proc.communicate()
        ret = proc.returncode

        if ret == 0:
            passed.add(path)
            return
        else:
            print('{0} unable to compile!'.format(base))
    except OSError:
        print('failed to execute {0}'.format(path))


def do_xpass(base, path):
    print('{0}'.format(path))
    tried.add(path)
    try:
        proc = subprocess.Popen(
            [DRIVER, path],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)

        out, err = proc.communicate()
        ret = proc.returncode

        if ret != 0:
            print('{0} returned error code {1}!'.format(base, ret))
            print('{0}'.format(err))
            return

        wanted = get_expected(path)
        if wanted in out:
            passed.add(path)
        else:
            print('{0} failed!'.format(base))
            print('got ----\n{0}\n--------'.format(out.strip()))
            print('exp ----\n{0}\n--------'.format(wanted))
    except OSError:
        print('failed to execute {0}'.format(path))


def do_xfail(base, path):
    print('{0}'.format(path))
    tried.add(path)
    try:
        proc = subprocess.Popen(
            [DRIVER, path],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)

        out, err = proc.communicate()
        ret = proc.returncode

        if ret != 0:
            passed.add(path)
            return
        else:
            print('{0} unexpected pass!'.format(base))
    except OSError:
        print('failed to execute {0}'.format(path))


def main():
    arg_fast = ('-fast' in sys.argv)

    for f in os.listdir('./xpass'):
        root, ext = os.path.splitext(f)
        if ext == '.ccml':
            do_xpass(root, os.path.join('./xpass', f))

    for f in os.listdir('./regression'):
        root, ext = os.path.splitext(f)
        if ext == '.ccml':
            do_xpass(root, os.path.join('./regression', f))

    for f in os.listdir('./xfail'):
        root, ext = os.path.splitext(f)
        if ext == '.ccml':
            do_xfail(root, os.path.join('./xfail', f))

    if not arg_fast:
        for f in os.listdir('./fuzz'):
            root, ext = os.path.splitext(f)
            if ext == '.ccml':
                do_comp(root, os.path.join('./fuzz', f))

    if not arg_fast:
        for f in os.listdir('./fuzzfail'):
            root, ext = os.path.splitext(f)
            if ext == '.ccml':
                try_comp(root, os.path.join('./fuzzfail', f))

    print('{0} of {1} passed'.format(len(passed), len(tried)))

    if len(passed) != len(tried):
        print('failed:')
        for x in tried:
            if x not in passed:
                print('   {0}'.format(x))
        exit(1)
    else:
        exit(0)


if __name__ == '__main__':
    main()
