import os
import subprocess
import sys


is_linux = (sys.platform == "linux" or sys.platform == "linux2")


if is_linux:
    DRIVER = '../build/ccml_driver'
else:
    DRIVER = '../build/Debug/ccml_driver.exe'


expected = {
    "array001" : "55",
    "array002" : "testcase",
    "array003" : "testcase",
    "const001" : "1240",
    "const002" : "3",
    "const003" : "13",
    "const004" : "correct",
    "eol001"   : "1",
    "fib9"     : "34",
    "gcd"      : "9",
    "global001": "9",
    "global002": "9",
    "string001": "101",
    "string002": "my string",
    "test001"  : "1234",
    "test002"  : "Hello World",
    "test003"  : "Hello World",
    "test004"  : "0123456789",
    "none001"  : "1",
    }


tried = set()
passed = set()


def do_run(base, path):
    tried.add(base)
    try:
        proc = subprocess.Popen(
            [DRIVER, path],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)

        out, err = proc.communicate()
        ret = proc.returncode

        if ret != 0:
            print '{0} returned error code {1}!'.format(base, ret)
            return

        if base in expected:
            wanted = expected[base]
            if wanted in out:
                passed.add(base)
            else:
                print '{0} failed!'.format(base)
                print 'got ----\n{0}\n--------'.format(out.strip())
                print 'exp ----\n{0}\n--------'.format(wanted)
    except OSError:
        print 'failed to execute {0}'.format(path)


def main():
    # compile all files in here
    for f in os.listdir('.'):
        root, ext = os.path.splitext(f)
        if ext == '.ccml':
            do_run(root, f)

    print '{0} of {1} passed'.format(len(passed), len(tried))

    if len(passed) != len(tried):
        print 'failed:'
        for x in tried:
            if x not in passed:
                print '   {0}'.format(x)
        exit(1)
    else:
        exit(0)

if __name__ == '__main__':
    main()
