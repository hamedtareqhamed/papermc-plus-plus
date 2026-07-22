# Minecraft Java Edition Protocol Reference

This document serves as the operational source-of-truth reference for implementing the Minecraft Java Edition protocol in PaperMC++.

---

## 1. Primary Specification Sources
* **Wiki.vg Protocol Reference:** [https://wiki.vg/Protocol](https://wiki.vg/Protocol)
* **PaperMC Core Repository:** [https://github.com/PaperMC/Paper](https://github.com/PaperMC/Paper)
* **Minecraft Data Schemas:** [https://github.com/PrismarineJS/minecraft-data](https://github.com/PrismarineJS/minecraft-data)

---

## 2. VarInt and VarLong Encoding Math

VarInts and VarLongs are variable-length 7-bit payload integers where the most significant bit (MSB 0x80) indicates whether additional bytes follow.

### VarInt Structure
- Value range: Signed 32-bit integer (-2147483648 to 2147483647).
- Encoded size: 1 to 5 bytes.

```cpp
// Encodes a 32-bit integer into a VarInt buffer
inline std::size_t encode_varint(int32_t value, std::span<uint8_t> out) {
    uint32_t uval = static_cast<uint32_t>(value);
    std::size_t count = 0;
    do {
        uint8_t temp = uval & 0x7F;
        uval >>= 7;
        if (uval != 0) {
            temp |= 0x80;
        }
        out[count++] = temp;
    } while (uval != 0);
    return count;
}
```

---

## 3. Protocol State Machine & Key Packet IDs

Minecraft connections transition through 4 explicit protocol states:
1. **Handshake (State 0)**
2. **Status (State 1)**
3. **Login (State 2)**
4. **Play (State 3)**

### Handshake Packets (State 0)
| Packet ID | Direction | Name | Fields |
|-----------|-----------|------|--------|
| `0x00`    | Serverbound | Handshake | `Protocol Version` (VarInt), `Server Address` (String), `Server Port` (Unsigned Short), `Next State` (VarInt: 1 for Status, 2 for Login) |

### Status Packets (State 1)
| Packet ID | Direction | Name | Description |
|-----------|-----------|------|-------------|
| `0x00`    | Serverbound | Status Request | Triggers Server List Ping JSON response |
| `0x00`    | Clientbound | Status Response | JSON Payload containing server MOTD, player counts, and icon |
| `0x01`    | Serverbound | Ping Request | Contains long payload timestamp |
| `0x01`    | Clientbound | Ping Response | Echoes payload timestamp |

### Login Packets (State 2)
| Packet ID | Direction | Name | Description |
|-----------|-----------|------|-------------|
| `0x00`    | Serverbound | Login Start | Client username and UUID |
| `0x01`    | Serverbound | Encryption Response | Shared secret and verify token encrypted with RSA |
| `0x02`    | Clientbound | Login Success | Sent when authentication succeeds; transitions connection to Play state |
| `0x03`    | Clientbound | Set Compression | Enables Zlib threshold compression |

---

## 4. Chunk & Block Data Layout (Protocol Format)

Each Chunk Column (16x384x16) contains sub-chunk sections of 16x16x16 blocks.
Each section contains:
1. **Block Count:** `int16_t` number of non-air blocks.
2. **Block States Palette:**
   - Single Value Palette (bits per entry = 0)
   - Indirect Palette (bits per entry = 4..8 with palette ID list)
   - Direct Palette (bits per entry = 15+, global block state IDs)
3. **Bit Storage Array:** Packed long array holding palette indices per block.
