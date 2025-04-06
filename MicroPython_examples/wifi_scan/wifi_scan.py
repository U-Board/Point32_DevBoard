import network
wifi = network.WLAN(network.STA_IF)
wifi.active(True)
wifi.disconnect()
aps = wifi.scan()
for ap in aps:
    print(ap)