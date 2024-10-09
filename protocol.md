# Protocol Proposal

## Init
**Code:** `0xFF`

###  Raspberry Pi:
Must:
- Specify Nucleo adress
- Specify version, subversion
- Specify frequency of heartbeat (in ms)



#### Packet Structure:
| Byte  | Content           |
|-------|-------------------|
| `0xFF`| Init code          |                                           
| `0x00`| Address            |                                           
| `0x01`| Version            |                                           
| `0x01`| Sub-version        |                                          
| `0x01` | Frequency entry | 

Frequency table:

| Byte (Hex) | Frequency (ms) |
|------------|----------------|
| 0x00       | 250 ms         |
| 0x01       | 500 ms         |
| 0x02       | 750 ms         |
| 0x03       | 1000 ms        |
| .. | .. |
| 0xFF | 63750 ms| 


### Nucleo:
Must:
- Specify adress
- Specify version
- Specify state

#### Packet Structure:
| Byte  | Content           | Description                               |
|-------|-------------------|-------------------------------------------|
| `0xFF`| Init code          |                                           |
| `0x01`| Address            |                                           |
| `0x01`| Version            |                                           |
| `0x01`| Sub-version        |                                           |
| `0x??`| Positive or negative response | Based on the initialization    |

`0x00` is the only positive response

### Init Faults:
#### Raspberry Pi Init Faults:
- `0x01` Old Version (Raspberry Pi)
- `0x02` Serial not opened 
- `0x03` Nucleo does not answer
- Others?
#### Nucleo:
- `0x10` Old Version (Nucleo)
- `0x20` Thrusters fail initialization
- Others?


## Communication
**Code:** `0xAA`

#### Packet Structure:
| Byte  | Content           | Description                               |
|-------|-------------------|-------------------------------------------|
| `0xAA`| Code              | Communication code                       |
| `0x00`| Command           | `0` for motor, `1` for arm                |
| `0x00`| Address           |                                           |
| | Arguments (dynamic)| Arguments, variable based on command     |
| | Checksum          | Checksum from Raspberry Pi                    |
| `0x0D`| End of packet     |                                           |

In particular:
- Command 0 (`motor`) has `8` arguments of `uint16_t` 
- Command 1 (`arm`) has `1` argument of `uint16_t` 

## Heartbeat
**Code:** `0xBB`

#### Packet Structure:
| Byte     | Content           | Description                               |
|----------|-------------------|-------------------------------------------|
| `0xBB`   | Code              | Heartbeat code                            |
| `0x00` | Address           |                                           |
| `0b1`  | Status            | `0` -> Not working, `1` -> OK             |
| `0b000`  | Status code       |                                           |

### Status Codes:
#### OK Codes:
- `000`: Ready
- `001`: Processing PWM
- ..

#### Not Working Codes:
- `000`: Not ready
- `001`: Thruster fail
- ...


