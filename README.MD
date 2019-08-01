- How to build

1. To build just run ```make``` and this will compile the code inside docker. The generated binary file ```vosion``` can be found under ```./build/armhf/vosion``` folder.
2. Copy ```vosion``` file to target board using ```scp``` command. Also copy configuration file:
```
scp ./build/armhf/vosion  root@192.168.1.45:~/
scp ./config/config.json root@192.168.1.45:~/
```
- How to run
1. SSH in to target board by: ```ssh root@192.168.1.45```.
2. Make sure both binary file and config file are copied to the target board.
3. Run ```./vosion```.
- How to config
Modify config.json file if necessary. Find the section that configs the analog sensor:
```
"analog_sensors": {
    "sample_number": 100,
    "sample_interal": 10,
    "read_interval": 1000,
    "pressure": {
      "device_num": 0,
      "channel_num": 1
    },
    "flow": {
      "device_num": 1,
      "channel_num": 2
    }
  }
  ```
  Change ```read_interval``` to reflect the sample interval in miliseconds. Chage the device number and channel nubmer for ```pressure``` sensor.
