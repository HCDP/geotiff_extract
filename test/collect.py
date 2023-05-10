from os import listdir, getcwd
from os.path import join

path = join(getcwd(), "files")
dir_list = listdir(path)

with open("test.txt", "w") as f:
    for item in dir_list:
        fpath = join(path, item)
        f.write(fpath + "\n")
