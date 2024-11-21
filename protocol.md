# Protocol Proposal (Chimpanzee)

## Init
**Code:** `0xFF`

###  Raspberry Pi -> Nucleo:
Must:
- Specify Nucleo adress
- Specify version, subversion
- Specify frequency of heartbeat (in ms)



#### Packet Structure:
| Byte  | Content           | Description|
|-------|-------------------|------------|
| `0xFF`| Init code          ||                                           
| `0x00`| Address            | |     
| `0x01`| Version  |  |
| `0x01`|Sub-version  ||                                        
| `0x01` | Interval entry | See [Interval table](interval-table)|
| `0x97` | CRC-8 ||
| `0xEE` | End of packet | Optional due init fixed dimension|

### Nucleo -> Raspberry Pi:
Must:
- Specify adress
- Specify version
- Specify state

#### Packet Structure:
| Byte  | Content           | Description                               |
|-------|-------------------|-------------------------------------------|
| `0xFF`| Init code          |                                           |
| `0x00`| Address            |      In case of multiple Nucleo                               |
| `0x01`| Version            |                                           |
| `0x01`| Sub-version        |                                           |
| `0x??`| Positive or negative response | Based on the initialization    |
| `0x??` | CRC-8|
| `0xEE` | End of packet |

`0x00` is the only positive response

#### Init Faults:
- `0x01` Serial not opened 
- `0x02` Old Version (Raspberry Pi)
- `0x03` Old Version (Nucleo)
- `0x04` Nucleo does not answer
- `0x05` Invalid answer
- `0x06` CRC failed
- `0x07` Thrusters fail initialization
- Others?

## Communication
**Code:** `0xAA`

### Packet Structure:
| Byte  | Content           | Description                               |
|-------|-------------------|-------------------------------------------|
| `0xAA`| Code              | Communication code                       |
| `0x00`| Address           |    In case of multiple Nucleo|
| `0x00`| Command           | See list below |
| | Arguments (dynamic)| Arguments, variable based on command     |
| | CRC-8    |   See [CRC-8 calculation rules]()                  |
| `0xEE`| End of packet     |                                           |


In particular:
- Command 0 (`motor`) has `8` arguments of `uint16_t` (Unidirectional)
- Command 1 (`arm`) has `1` argument of `uint16_t` (Unidirectional)
- Command 2 (`sensor`) see [sensor polling command](sensor-polling-command)

#### Sensor polling command
#####  Raspberry Pi -> Nucleo:


###### Payload Structure:
| Byte     | Content           | Description                               |
|----------|-------------------|-------------------------------------------|
| `0x00` | Sensor_ID          | ID for the sensor Nucleo must query |
| `0x76` | I<sup>2</sup>C  address        |  |
| `0x00` | Interval entry     | See [Interval table](interval-table) |
| `0x00`| Type of sensor | See [type sensor list](type-sensor-list)|

##### Nucleo -> Raspberry Pi:


###### Payload Structure:
| Byte     | Content           | Description                               |
|----------|-------------------|-------------------------------------------|
| `0x00` | Sensor_ID          | ID of queried sensor |
| `0x00`| Type of sensor | See [type sensor list](type-sensor-list)|
|  | Sensor data | According to [type sensor list](type-sensor-list)|





## Heartbeat (Only from Nucleo to Raspberry)
**Code:** `0xBB`

### Packet Structure:
| Byte     | Content           | Description                               |
|----------|-------------------|-------------------------------------------|
| `0xBB`   | Code              | Heartbeat code                            |
| `0x00` | Address           |                                           |
| `0b1`  | Status            | `0` -> OK, `1` -> Not Working             |
| `0b0000000`  | Status code       |                                           |
| `0x??`| Paylod| According to [Salvatore](https://github.com/Realshoresupply)|
| `0xEA` | CRC | | 
| `0xEE` | End of packet |

### Status Codes:
#### OK Codes:
- `000`: Ready
- `001`: Processing PWM
- ..

#### Not Working Codes:
- `000`: Not ready
- `001`: Thruster fail
- ...


### Byte stuffing
Behind each START_BYTES (`0xAA`, `0xBB` and `0xFF`), it is mandatory to prepend `0x7E` as ESCAPE CHARACTER.

Behind each `0xEE` byte, it is mandatory to prepend `0x7E` as ESCAPE CHARACTER.

Behind each `0x7E` byte, it is mandatory to prepend `0x7E` as ESCAPE CHARACTER.

### CRC-8 Calculation Rules

The CRC calculation must consider all the bytes in the packet, excluding:
- ESCAPE CHARACTER(S)
- END OF PACKET BYTE


### Interval table
Interval table:

| Byte (Hex) | Interval (ms) |
|------------|----------------|
| 0x00       | 10 ms         |
| 0x01       | 20 ms         |
| 0x02       | 30 ms         |
| 0x03       | 40 ms        |
| .. | .. |
| 0xFF | 2550 ms| 

## Sequence diagram

<img src="Chimpanzee.svg" alt="Diagram description" width="500"/>
