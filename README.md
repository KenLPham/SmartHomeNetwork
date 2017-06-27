# SmartHomeNetwork
By Kenneth Pham

Code used to create a network using RFM69 transceivers to turn on and off lights through WiFi. If you would like to make one yourself, I wrote an [Instructables](https://www.instructables.com/id/Smart-Home-Network-With-ESP8266-and-RFM69/) on how to make one.

## How does it work
When everything is setup, the ESP8266 will send a webpage when requested by a browser. This will send the name of the base and load the connected nodes with a toggle button. The base gets the information from the nodes at startup as the nodes will continuously try and send the data to the base every five seconds. When the data is finally sent, it is stored into an array. This array is used later to send the name and id to the webpage. When the toggle button is press, a POST request with the node id is sent to the ESP. The data sent by the request will be used to send a generic character to the node. When the node receives this generic character from the base, it will either switch on or off the lights.

## License
GPL 3.0, please see the [LICENSE](https://github.com/KenLPham/SmartHomeNetwork/blob/master/LICENSE) file for details. Be sure to include the same license with any fork or redistribution of this library.

## Libraries Used
* [LowPowerLab's RFM69 Library](https://github.com/LowPowerLab/RFM69)
