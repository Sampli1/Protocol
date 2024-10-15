# Protocol Proposal (Chimpanzee)

## Abstract Description

### Init
**Code:** `0xFF`

####  Raspberry Pi:
Must:
- Specify Nucleo adress
- Specify version, subversion
- Specify frequency of heartbeat (in ms)



##### Packet Structure:
| Byte  | Content           |
|-------|-------------------|
| `0xFF`| Init code          |                                           
| `0x00`| Address            |                                           
| `0x01`| Version            |                                           
| `0x01`| Sub-version        |                                          
| `0x01` | Frequency entry | 
| `0x97` | CRC-8 |
| `0xEE` | End of packet |

Frequency table:

| Byte (Hex) | Frequency (ms) |
|------------|----------------|
| 0x00       | 250 ms         |
| 0x01       | 500 ms         |
| 0x02       | 750 ms         |
| 0x03       | 1000 ms        |
| .. | .. |
| 0xFF | 63750 ms| 


#### Nucleo:
Must:
- Specify adress
- Specify version
- Specify state

##### Packet Structure:
| Byte  | Content           | Description                               |
|-------|-------------------|-------------------------------------------|
| `0xFF`| Init code          |                                           |
| `0x01`| Address            |                                           |
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

### Communication
**Code:** `0xAA`

#### Packet Structure:
| Byte  | Content           | Description                               |
|-------|-------------------|-------------------------------------------|
| `0xAA`| Code              | Communication code                       |
| `0x00`| Command           | `0` for motor, `1` for arm                |
| `0x00`| Address           |                                           |
| | Arguments (dynamic)| Arguments, variable based on command     |
| | CRC-8    |                     |
| `0xEE`| End of packet     |                                           |

In particular:
- Command 0 (`motor`) has `8` arguments of `uint16_t` 
- Command 1 (`arm`) has `1` argument of `uint16_t` 

### Heartbeat
**Code:** `0xBB`

#### Packet Structure:
| Byte     | Content           | Description                               |
|----------|-------------------|-------------------------------------------|
| `0xBB`   | Code              | Heartbeat code                            |
| `0x00` | Address           |                                           |
| `0b1`  | Status            | `0` -> Not working, `1` -> OK             |
| `0b0000000`  | Status code       |                                           |
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
Behind each `0xEE` byte, it is mandatory to prepend `0x7E` as ESCAPE CHARACTER.

Behind each `0x7E` byte, it is mandatory to prepend `0x7E` as ESCAPE CHARACTER.

**IMPORTANT:** THE CRC CALCULATION **MUST EXCLUDE** ESCAPE CHARACTERS! 