# espMqttIrblaster
/*
* This device has two functions:
* - Measure light level with an IR-transistor and send the value via Mqtt.
* - Receive Mqtt messages and sends IR codes to televisions  and other stuff with ir-receivers. 
* 
* This makes it possible to use a command, an app or anything that can send Mqtt messages to control 
* a recevier, a TV or just anything that can be controlled via Ir. I use it with my LG TV remote app, 
* https://github.com/bphermansson/LgMqttremote
* * 
* Send Mqtt messages like this "Manufacturer code, ir code, code length"
* Manufacturer code are 1 for Samsung, 2 for LG, 3 for Yamaha. More can be added if you want. 
* 
* Ir code 
* Find your remote at http://lirc.sourceforge.net/remotes/
* Note bits, pre_data_bits, pre_data and a code
* Example, Samsung BN59-00538A. 
* bits = 16, pre_data_bits = 16, pre_data = 0xE0E0, power on/off code = 0x40BF
* 
* Then the message to send is manufacturer code, pre_data+code, pre_data_bits+bits, longpress (0 or 1) =
* "1,E0E040BF,32,1"
* 
* Longpress gives a longer transmission, sometimes needed to turn of equipment.
* 
* You can test with the mosquitto command:
* 
* LG TV On:
* mosquitto_pub -h 192.168.1.79 -u 'emonpi' -P 'emonpimqtt2016' -t 'irsender' -m '2,20DF10EF,32'
*
*
* PHermansson 20161018
*/
