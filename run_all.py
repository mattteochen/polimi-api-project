import os
import filecmp

ERROR = "[ERROR]: "
INFO  = "[INFO] : "

ret = os.system("clear && gcc -std=c11 main.c")
if ret:
    print(ERROR + "compilation error")
    exit(1)

tests = os.listdir("./tests")

ins = {}
for test in tests:
    if not test.endswith(".output.txt") and not test.endswith(".DS_Store"):
        ins[f'{test[0:len(test)-len(".txt")]}'] = {"in": test, "out": test[0:len(test)-len(".txt")]+".output.txt"}

for test in ins:
    print(ins[test]["in"])
    ret = os.system("./a.out <" + f'./tests/{ins[test]["in"]}' + "> out.txt")
    if ret:
        print(ERROR + "Failed to run test: " + test)
    print(INFO + "Test: " + test + " result: " + str(filecmp.cmp(f'./tests/{ins[test]["out"]}', "out.txt", shallow=False)))
    os.system(f'mv out.txt out/{test}_out.txt')


