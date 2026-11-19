export const initialBom = { revision: 5, display: 'EX-DISP50', ledCurrent: null };
export const displays = {
  'EX-DISP50': { voltage: 5, current: 80 },
  'EX-DISP33': { voltage: 3.3, current: 50 }
};

export function parts(bom) {
  const display = displays[bom.display];
  return [
    ['U1', 'EX-MCU32', 'Microcontroller', '3.3 V · 48 MHz', 'Root controller', 40],
    ['U4', 'EX-I2CMUX8', 'I²C multiplexer', '8 channels · 0x70', 'U1 · PA0 / PA1', 2],
    ['U2', 'EX-TEMP33', 'Temperature sensor', '3.3 V · 0x48', 'U4 · channel 0', 10],
    ['U3', bom.display, 'Display module', display.voltage + ' V MISO · SPI', 'U1 · PB0–PB4', display.current],
    ['C1', 'USER-LED2', 'Status-light circuit', '3.3 V · GPIO', 'U1 · PC0 / PC1', bom.ledCurrent]
  ];
}
