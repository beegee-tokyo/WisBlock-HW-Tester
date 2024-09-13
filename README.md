| <img src="./assets/RAK-Whirls.png" alt="RAKWireless"> | <img src="./assets/meshtastic.png" alt="Meshtastic" width=30%> | <img src="./assets/rakstar.jpg" alt="RAKstar" > |    
| :-: | :-: | :-: |     

# WisBlock-HW-Tester
Test application to do a basic hardware test with WisBlock Base Boards, WisBlock Core RAK4631 and WisBlock modules and displays.

## Why we need a WisBlock HW tester
While doing customer support for WisBlock based devices, it is often unclear whether a problem is related to the BaseBoard, Core module or any WisBlock IO or sensor module attached.

## Origin of this WisBlock HW testers
This hardware tester application was created to make our work with the many WisBlock support request for devices running with Meshtastic firmware. As RAKwireless is not developing or maintaining the Meshtastic firmware it is often difficult to find out whether the problem is related to the WisBlock hardware or to a problem in the Meshtastic firmware.

## What is tested
This simple application is testing basic hardware functions of a WisBlock Base device:

- BaseBoard LED's 
- RAK1921 OLED display if connected
- RAK14000 EPD display if connected
- Check for connected I2C devices
- Read/Write test on the nRF52840 flash memory
- Basic LoRa transceiver check
- Check the analog input for the battery status reading
- Check RAK12500 GNSS location module if connected (and if test is done outdoors)

The results of the tests are sent over the USB port and if any display is attached, are shown on the display as well.


----
----
# Meshtastic® is a registered trademark of Meshtastic LLC.    
Meshtastic software components are released under various licenses, see GitHub for details. No warranty is provided - use at your own risk.

# LoRa® is a registered trademark or service mark of Semtech Corporation or its affiliates. 

# LoRaWAN® is a licensed mark.

----
----