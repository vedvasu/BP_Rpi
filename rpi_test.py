import cv2
import numpy as np
import time
import RPi.GPIO as GPIO

GPIO.setmode (GPIO.BCM)                                       #configuring GPIO pins for forward motion
GPIO.setwarnings (False)
GPIO.setup (18, GPIO.IN)

z = GPIO.input(18)

print z

