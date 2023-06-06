import os
import filecmp
import sys
import time

ERROR = "[ERROR]: "
INFO  = "[INFO] : "


class ColorPrint:

    @staticmethod
    def print_fail(message, end = '\n'):
        sys.stderr.write('\x1b[1;31m' + message.strip() + '\x1b[0m' + end)

    @staticmethod
    def print_pass(message, end = '\n'):
        sys.stdout.write('\x1b[1;32m' + message.strip() + '\x1b[0m' + end)

    @staticmethod
    def print_warn(message, end = '\n'):
        sys.stderr.write('\x1b[1;33m' + message.strip() + '\x1b[0m' + end)

    @staticmethod
    def print_info(message, end = '\n'):
        sys.stdout.write('\x1b[1;34m' + message.strip() + '\x1b[0m' + end)

    @staticmethod
    def print_bold(message, end = '\n'):
        sys.stdout.write('\x1b[1;37m' + message.strip() + '\x1b[0m' + end)

ret = os.system("clear && gcc -std=c11 main.c")

if ret:
    ColorPrint.print_fail(ERROR + "compilation error")
    exit(1)

tests = os.listdir("./tests")

ins = {}
for test in sorted(tests):
    if test.endswith(".txt"):
        if not test.endswith(".output.txt"):
            ins[f'{test[0:len(test)-len(".txt")]}'] = {"in": test, "out": test[0:len(test)-len(".txt")]+".output.txt"}

start_time = time.time()
for test in ins:
    ColorPrint.print_info(f"\n\n------------  {ins[test]['in']} ------------")
    ret = os.system("./a.out <" + f'./tests/{ins[test]["in"]}' + "> out.txt")
    if ret:
        ColorPrint.print_warn(ERROR + "Failed to run test: " + test)
    else:
        compare = str(filecmp.cmp(f'./tests/{ins[test]["out"]}', "out.txt", shallow=False))
        if compare == "True":
            ColorPrint.print_pass(INFO + "Test: " + test + " result: " + compare)
        else:
            ColorPrint.print_fail(INFO + "Test: " + test + " result: " + compare)
    os.system(f'mv out.txt out/{test}_out.txt')

print("Total Running time = {:.3f} seconds".format(time.time() - start_time))
