from tifffile import TiffFile
from time import time

start = time()
with TiffFile("rainfall_new_month_statewide_data_map_2023_02.tif") as tif:
    data = tif.get_data()
print(time() - start)