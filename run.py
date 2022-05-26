import os

ret = os.system("clear && gcc -std=c11 main.c")
if (ret):
    exit(1)
ret = os.system("./a.out")
exit(ret)
