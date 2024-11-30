import os
import sys

if len(sys.argv) <= 1:
    sys.exit(0)

pth = sys.argv[1]
for file in os.listdir(pth):
    print(file)
    fp = os.path.join(pth, file)
    if os.path.isdir(fp):
        continue
    elif os.path.islink(fp):
        os.remove(fp)
    elif file == "OpenAL32.dll":
        os.rename(fp, fp.replace("OpenAL32.dll", "openal.dll"))
    elif file == "glfw3.dll":
        os.rename(fp, fp.replace("glfw3.dll", "glfw.dll"))
    else:
        filenameparts = file.split(".")
        resultparts = []
        for i in range(len(filenameparts)):
            try:
                test = int(filenameparts[i])
            except BaseException as e:
                resultparts.append(filenameparts[i])
        os.rename(fp, os.path.join(pth, ".".join(resultparts)))
