# BP_Rpi
Measurement of blood pressure using near infrared spectroscopy through Rpi-python

- The raspberry pi SOC is used to get the dateset for the blood pressure pulses through IR sensors
- If you have the data for IR readings the code can run on any machine with python.

Connecting IR module to Rpi:

- Get a IR sensor module of simple a TSOP IR receiver
- Connect the Vcc pin of module to pin 2 in Rpi (Vcc pin)
- Connect the ground pin of module to pin 6 in Rpi (Gnd pin)
- Connect the data pin of module to pin 12 in Rpi (GPIO 18)
	(can be changed, but Rpi configuration will change)

Configuring Raspberry Pi (After setting up Rpi):

- Install LIRC module to enable IR functionality
	sudo apt-get install lirc

- Open the /etc/modules file: 
	sudo nano /etc/modules
   
  and add these lines at the end to make LIRC start up on boot and set the IR sensor pin to Pin-18 and IR LED pin(for later) to Pin-17:
	lirc_dev
	lirc_rpi gpio_in_pin=18 gpio_out_pin=17 

- Edit the LIRC hardware configuration file. Open it using:
	sudo nano /etc/lirc/hardware.conf

  Change the following lines:
	DRIVER="default"
	DEVICE="/dev/lirc0"
	MODULES="lirc_rpi"

- To make it work, you need to reboot your Raspi once:
	sudo reboot

-  Edit /boot/config.txt using:
	sudo nano /boot/config.txt

  add the following line to it:
	dtoverlay=lirc-rpi,gpio_in_pin=18,gpio_out_pin=17,gpio_in_pull=up

- For Testing use following commands on terminal:
	sudo /etc/init.d/lirc stop
	mode2 -d /dev/lirc0
  This will show pulse/source with a integer...infinitely running CTRL + C to stop

Running the code and getting blood pressure output.

Step1: Copy irtest.py and irtest_c.py in the Rpi directory (say desktop)

Step2: Disconnect the data pin 12 and Run irtest.py
	cd Desktop
	python irtest.py

Step3: Create dataset
while the code above is running place the IR sensor on your arm near the joint
and connect the datapin 12 for 1 sec and disconnect it

now place the IR sensor at your finger
and connect the datapin 12 for 1 sec and disconnect it

press CTRL+C to end the above running code.

Step4: Results; run python code irtest_c.py
	python irtest_c.py

The blood pressure is systolic value.
	
	