# M64

Movies (.m64 files, also called "m64s") are the storage formats for TASes created using Mupen64.

They are made up of a 1024-byte long header, followed by a variable-size sequence of input samples.

| Type | Name | Additional Info |
| - | - | - |
| uint8_t[4] | Magic | Must be `0x4D36341A`
| uint32_t | Version | Must be `3`
| uint32_t | UID | Used to correlate a movie with its savestate
| uint8_t | VI/s | The amount of VIs generated per second.<br>Can also be determined using the country code.
| uint8_t | Controller count | The amount of connected controllers<br>Must be lower than or equal to `4`<br>Can also be determined using the controller flags.
| uint8_t | Extended version | The version number of the extended section.<br>See extended format section.
| uint8_t | Extended flags | The extended flags.<br>See extended format section.
| uint32_t | Sample count | Number of input samples. Determines the element count of the input stream beginning at offset `1024`.
| uint16_t | Start type | The movie start type.<br>`1` - Start from savestate<br>`2` - Start from reset<br>`4` - Start from EEPROM
| uint8_t[2] | Reserved | Must be zero-filled |
| uint32_t | Controller flags | The controller flags, a bitfield containing data for each controller sequentially.<br>`n` - Present<br>`n+4` - Mempak<br>`n+8` - Rumblepak<br>Substitute `n` for the controller index (relative to 0). |
| uint8_t[32] |  Extended data | The extended data.<br>See extended format section. |
| uint8_t[128] | Reserved | Must be zero-filled |
| char[32] (ASCII) | ROM Name | The name of the ROM the movie was recorded on |
| uint32_t | ROM CRC | The CRC of the ROM the movie was recorded on |
| uint16_t | ROM Country Code | The country code of the ROM the movie was recorded on |
| uint8_t[56] | Reserved | Must be zero-filled |
| char[64] (ASCII) | Video Plugin Name | The name of the video plugin used during movie recording |
| char[64] (ASCII) | Audio Plugin Name | The name of the audio plugin used during movie recording |
| char[64] (ASCII) | Input Plugin Name | The name of the input plugin used during movie recording |
| char[64] (ASCII) | RSP Plugin Name | The name of the RSP plugin used during movie recording |
| char[222] (UTF-8) | Author | The movie's author |
| char[256] (UTF-8) | Description | Additional info about the movie |
| uint32_t[sample_count] | Input data | Array of input data of element count `Sample count`<br>See input sample section.

## Input Sample

An input sample consists of 32 bits and represents the input state for a specific frame.

It is structured as follows:

| Bit length | Name | Additional info |
| - | - | - |
| 1 | C-Right |
| 1 | C-Left |
| 1 | C-Down |
| 1 | C-Up |
| 1 | R |
| 1 | L |
| 1 | Reserved (1) | If both reserved fields are set, the emulator will reset on that frame.
| 1 | Reserved (2) |
| 1 | D-Right |
| 1 | D-Left |
| 1 | D-Down |
| 1 | D-Up |
| 1 | Start |
| 1 | Z |
| 1 | B |
| 1 | A |
| 8 | Joystick X | Interpreted as `int8_t`
| 8 | Joystick Y | Interpreted as `int8_t`

## Extended Format

The extended format is an updated version of the M64 format.

It keeps the same version number for compatibility reasons, but defines new data in the data gaps of the old format.

### Version mapping

The mapping of Mupen64 versions to extended version numbers is as follows:

| Mupen version | Extended version |
| - | - |
| < 1.1.9 | 0 |
| 1.1.9 | 1 |
| 1.1.9-x | 1 |

### Flags

| Bit | Name | Additional Info |
| - | - | - |
| 0 | WiiVC | Whether the movie was recorded with WiiVC emulation mode enabled.<br>Valid on versions: `1`
| Rest | Reserved | Must be 0, or a warning will be shown on playback. |

# ST

See https://github.com/Eddio0141/Mupen64-ST-file-format.