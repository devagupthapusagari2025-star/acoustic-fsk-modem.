# Acoustic FSK Modem

Two-tone FSK data transfer using Goertzel tone detection on an Arduino Uno,
with CRC-8 protected framing. Simulated in Wokwi.

**Live simulation:** https://wokwi.com/projects/477291068177901569

## How it works
- Bit 0 = 1000 Hz tone, bit 1 = 2000 Hz tone
- Receiver samples the signal and uses the Goertzel algorithm to decide which tone it heard
- Frame format: [preamble][start byte][length][payload][CRC-8]
- Simulation channel: a wire from D8 to A0 stands in for the air

## Results
(Add your BER graphs here after Step 6)

## Limitations
- The simulated channel is electrical, not acoustic
- About 50 bits per second

## Files
- `step5_protocol/` - framing and CRC
- `step6_ber/` - noise and BER experiments
- `diagram.json` - Wokwi wiring
