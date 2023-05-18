from os import walk, getcwd
from os.path import join

path = join(getcwd(), "files")

with open("test.txt", "w") as f:
    for root, dirs, files in walk(path):
        for file in files:
            fpath = join(root, file)
            f.write(fpath + "\n")
