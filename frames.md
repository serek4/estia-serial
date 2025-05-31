# Estia frames
## Frame structure
```
<-header--> <---data header----> <-data-> <-crc->
00 01 02 03 04 05 06 07 08 09 10 11       -1 last
a0 00 xx xx xx xx xx xx xx xx xx xx xx xx xx xx
|| || || ||                               ^^ ^^ - CRC-16/MCRF4XX
|| || || ^^ - dataLength
|| || ^^ - frame type
^^ ^^ - frame begin
```
### Frame types
```
0x10 control frame
0x11 command
0x12 at boot
0x15 second remote?
0x17 data request
0x18 ack
0x1a data response
0x1c status data update
0x55 30s status often corrupted
0x58 19 30s status (long)
0x58 0b 30m status (short)
0x98 at boot
0x95 at boot
```
## Data
### Data header
```
<---data header----> <-data->
00 01 02 03 04 05 06 07
   || || || || ^^ ^^ - data type
   || || ^^ ^^ - destination
   ^^ ^^ - source
```
#### sources/destinations
```
00 fe - broadcast, dst only
08 00 - master
00 40 - remote
00 f0 - at boot only, dst only ?
00 a0 - at boot only, dst only ?
```
#### data types
```
00 8a - heartbeat
03 c6 - status
03 c4 - mode change
00 41 - operation switch
03 c1 - temperature change
00 15 - special command
00 80 - data request
00 ef - data response
00 a1 - ack
00 2b - short status (and ping?)
```
## Status frames
###  Heartbeat frame, `13` bytes
```
a0 00 10 07 00 08 00 00 fe 00 8a 75 05
```
### status frame, `31` bytes
```
a0 00 58 19 00 08 00 00 fe 03 c6 c1 30 10 78 5c 7a 78 5c 7a 00 00 00 00 00 e9 89 5e 00 41 4a
a0 00 58 19 00 08 00 00 fe 03 c6 c1 30 12 78 62 7a 78 5e 6c 00 10 00 00 00 e9 84 5d 00 d8 d0 - night mode on
a0 00 58 19 00 08 00 00 fe 03 c6 c2 24 18 78 60 7a 78 5e 55 00 10 00 00 00 e9 58 68 00 f1 4b
a0 00 58 19 00 08 00 00 fe 03 c6 c1 00 10 76 5c 7a 76 5c 7a 00 00 00 00 00 e9 7d 5a 00 34 e2
a0 00 58 19 00 08 00 00 fe 03 c6 c0 20 00 76 5a 7a 76 50 70 00 10 00 00 00 e9 5d 58 00 ca 49
a0 00 58 19 00 08 00 00 fe 03 c6 c0 00 00 76 5a 7a 76 5a 7a 00 00 00 00 00 e9 5d 58 00 a6 1c
                                 || || || || || || || || ||    ||             || ^^ 27 - TWI (0x68 / 0x02 - 0x10) ?
                                 || || || || || || || || ||    ||             ^^ 26 - TWO (0x58 / 0x02 - 0x10) ?
                                 || || || || || || || || ||    ^^ 21 - +0x02 defrost in progress, +0x10 night mode active
                                 || || || || || || || || ^^ 19 - follow zone 2 temperature, actual target temperature (including night setback)
                                 || || || || || || || ^^ 18 - follow zone 1 temperature, actual target temperature (including night setback)
                                 || || || || || || ^^ 17 - follow hot water temperature, actual target temperature (including boost) ?
                                 || || || || || ^^ 16 - zone 2 temperature 0x7a / 0x02 - 0x10 ?
                                 || || || || ^^ 15 - zone1 temperature 0x60 / 0x02 - 0x10
                                 || || || ^^ 14 - hot water temperature 0x76 / 0x02 - 0x10
                                 || || ^^ 13 - 0x00 -> off, +0x01 backup e-heater, +0x02 heating cmp on, +0x04 HW e-heater, +0x08 HW cmp on, +0x10 -> pump1 on
                                 || ^^ 12 - 0x00 off, +0x04 auto mode on, +0x10 quiet on, +0x20 night on
                                 ^^ 11 - 0xc0 all off, +0x01 heating on, +0x02 hot water on, 0xc3 heating + hot water
```
### status update frame, `21` bytes
```
a0 00 1c 0f 00 08 00 00 fe 03 c6 c1 00 12 76 60 7a 00 00 15 4c
a0 00 1c 0f 00 08 00 00 fe 03 c6 c0 20 00 76 5a 7a 10 00 b8 5b
                                 || || || || || || ^^ 17 - +0x02 defrost in progress, +0x10 night mode active
                                 || || || || || ^^ 16 - zone 2 temperature 0x7a / 0x02 - 0x10 ?
                                 || || || || ^^ 15 - zone1 temperature 0x60 / 0x02 - 0x10
                                 || || || ^^ 14 - hot water temperature 0x76 / 0x02 - 0x10
                                 || || ^^ 13 - 0x00 -> off, +0x01 backup e-heater, +0x02 heating cmp on, +0x04 HW e-heater, +0x08 HW cmp on, +0x10 -> pump1 on
                                 || ^^ 12 - 0x00 off, +0x04 auto mode on, +0x10 quiet on, +0x20 night on
                                 ^^ 11 - 0xc0 all off, +0x01 heating on, +0x02 hot water on, 0xc3 heating + hot water
```
### status frame, `15` bytes
```
a0 00 55 09 00 00 40 08 00 03 c6 00 00 ae 4c
```
### short status frame, `17` bytes
```
a0 00 58 0b 00 08 00 00 fe 00 2b 00 00 01 32 7c 58
```
## Data request
### data request frame, `21` bytes

requested data code byte `17`  

```
a0 00 17 0f 00 00 40 08 00 00 80 00 ef 00 2c 08 00 06 00 2c 40 -> request data, code offset 17, value = 0x00 - 0xff
                                 || ||             ^^ - requested data code
                                 ^^ ^^ - frame with this data type expected ?
```
### data response frame, `19` bytes
```
a0 00 1a 0d 00 08 00 00 40 00 ef 00 80 00 2c 00 1f 73 83
a0 00 1a 0d 00 08 00 00 40 00 ef 00 80 00 a2 00 2c 6c 6c
                                 || || || || ^^ ^^ - data for requested code (int16BE)
                                 || || ^^ ^^ - 0x002c -> data available, 0x00a2 -> data not available
                                 ^^ ^^ - response to frame with this type ?
```
## Commands
### `14` bytes

command byte `11`  

#### heating on/off
```
a0 00 11 08 00 00 40 08 00 00 41 23 8f 38             -> on,  0x23
a0 00 11 08 00 00 40 08 00 00 41 22 9e b1             -> off, 0x22
```
```
a0 00 11 08 00 08 00 08 00 00 41 03 72 07             -> on,  0x03 by digital input
a0 00 11 08 00 08 00 08 00 00 41 02 63 8e             -> off, 0x02 by digital input
```
#### hot water on/off
```
a0 00 11 08 00 00 40 08 00 00 41 2c 77 cf             -> on,  0x2c
a0 00 11 08 00 00 40 08 00 00 41 28 31 eb             -> off, 0x28
```
```
a0 00 11 08 00 08 00 08 00 00 41 0c  crc              -> on,  0x0c by digital input
a0 00 11 08 00 08 00 08 00 00 41 08 cc d4             -> off, 0x08 by digital input
```
### `17` bytes
command byte `11`  
value byte `12`  

#### auto mode off/on
```
a0 00 11 0b 00 00 40 08 00 03 c4 01 01 00 00 84 03    -> on,              command 0x01, value 0x01
a0 00 11 0b 00 00 40 08 00 03 c4 01 00 00 00 de df    -> off,             command 0x01, value 0x00
```
#### quiet mode on/off (glitched)

Quiet mode must be enabled in controller all the time to be able to switch on and off with commands.

```
a0 00 11 0b 00 00 40 08 00 03 c4 04 04 00 00 d3 e9    -> on,              command 0x04, value 0x04 (0x01 << 2)
a0 00 11 0b 00 00 40 08 00 03 c4 04 00 00 00 b0 88    -> off,             command 0x04, value 0x00
```
#### quiet mode activate/deactivate
```
a0 00 11 0b 00 00 40 08 00 03 c4 04 04 00 00 d3 e9    -> activate,        command 0x04, value 0x04 (0x01 << 2) ?
a0 00 11 0b 00 00 40 08 00 03 c4 04 00 00 00 b0 88    -> deactivate,      command 0x04, value 0x00 ?
```
#### night mode on/off
```
a0 00 11 0b 00 00 40 08 00 03 c4 88 08 00 00 cc 10    -> on,              command 0x88, value 0x08 (0x01 << 3)
a0 00 11 0b 00 00 40 08 00 03 c4 88 88 00 00 c0 fc    -> on and activate, command 0x88, value 0x88 ?
a0 00 11 0b 00 00 40 08 00 03 c4 88 00 00 00 0a d2    -> off,             command 0x88, value 0x00
```
#### night mode activate/deactivate
```
a0 00 11 0b 00 00 40 08 00 03 c4 88 88 00 00 c0 fc    -> activate,        command 0x88, value 0x88 (0x11 << 3) current | 0x80
a0 00 11 0b 00 00 40 08 00 03 c4 88 08 00 00  crc     -> deactivate,      command 0x88, value 0x08 (0x01 << 3) current & 0x08
```
#### combined `17` bytes frame

command `8c = 04 | 88` value   `0c = 04 | 08`

```
a0 00 11 0b 00 00 40 08 00 03 c4 8c 0c 00 00 dd 9d    -> combined quiet mode activate + night mode deactivate
```
### `16` bytes
#### force defrost

command byte `12`  
value byte `13`  

```
a0 00 11 0a 00 00 40 08 00 00 15 00 46 01 e7 25       -> on,              command 0x46, value 0x01
a0 00 11 0a 00 00 40 08 00 00 15 00 46 00 f6 ac       -> off,             command 0x46, value 0x00
```
### `18` bytes

command byte `11`

#### heating temperature change
```
a0 00 11 0c 00 00 40 08 00 03 c1 02 5c 7a 76 5c b2 d1 -> command 0x02, value offset 12 and 15, value = (temp + 16) * 2
```
#### heating zone 2 temperature change ?
```
a0 00 11 0c 00 00 40 08 00 03 c1 04 5c 7a 76 5c  crc  -> command 0x04, value offset 13, value = (temp + 16) * 2 ?
```
#### hot water temperature change
```
a0 00 11 0c 00 00 40 08 00 03 c1 08 00 12 78 5c e5 c4
a0 00 11 0c 00 00 40 08 00 03 c1 08 00 00 70 00 83 c0 -> command 0x08, value offset 14, value = (temp + 16) * 2
```
### `13` bytes

command byte `none`

#### empty command (ping?) every 30min
```
a0 00 11 07 00 00 40 08 00 00 2b 15 f6

a0 00 18 09 00 08 00 00 40 00 a1 00 2b ed b3 # ping frame ack (pong?)
```
### ack frame, `15` bytes
```
a0 00 18 09 00 08 00 08 00 00 a1 00 41 c1 95
                                 ^^ ^^ - frame with this data type ack'd
```
