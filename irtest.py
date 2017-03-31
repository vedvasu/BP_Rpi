import os
import time

os.system("sudo /etc/init.d/lirc stop")
os.system('mode2 -d /dev/lirc0 > dataset.txt')