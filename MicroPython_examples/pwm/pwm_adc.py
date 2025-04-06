from machine import Pin,PWM
import time

pwm =PWM(Pin(8,Pin.OUT),1000)

try:
    while True:
        pwm.duty(126)
        time.sleep_ms(100)
except:
    pwm.deinit()





