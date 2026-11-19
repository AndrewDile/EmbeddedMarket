-- Seed data for the Field Node demo. Every EX-* part is fictional example data.
-- USER-LED2 has no recorded current on purpose: the user supplies it during the flow.

INSERT INTO parts
    (part_number, name, role, supply_min_v, supply_max_v, logic_output_high_v, logic_input_high_min_v,
     logic_input_max_v, interfaces, available_gpio, extra_pins, current_ma, clock_mhz, flash_kb, ram_kb)
VALUES
    ('EX-MCU32',   'Microcontroller',      'controller', 3.00, 3.60, 3.30, 2.00, 3.30, 'I2C,SPI,UART,GPIO', 20,   0, 40.00, 48.00, 256,  64),
    ('EX-TEMP33',  'Temperature sensor',   'peripheral', 3.00, 3.60, 3.30, 2.00, 3.60, 'I2C',               NULL, 0, 10.00, NULL,  NULL, NULL),
    ('EX-I2CMUX8', 'I2C multiplexer',      'peripheral', 3.00, 3.60, 3.30, 2.00, 3.60, 'I2C',               16,   0,  2.00, NULL,  NULL, NULL),
    ('USER-LED2',  'Status-light circuit', 'peripheral', 3.00, 3.60, 3.30, 2.00, 3.60, 'GPIO',              NULL, 2,  NULL, NULL,  NULL, NULL),
    ('EX-DISP50',  'Display module',       'peripheral', 3.00, 5.50, 5.00, 2.00, 5.50, 'SPI',               NULL, 1, 80.00, NULL,  NULL, NULL),
    ('EX-DISP33',  'Display module',       'peripheral', 3.00, 3.60, 3.30, 2.00, 3.60, 'SPI',               NULL, 1, 50.00, NULL,  NULL, NULL);

INSERT INTO boms
    (id, name, supply_min_v, supply_max_v, power_budget_ma, allowed_interfaces, min_clock_mhz, min_flash_kb, min_ram_kb)
VALUES
    (1, 'Field node', 3.30, 3.30, 200.00, 'I2C,SPI,GPIO', 48.00, 256, 64);

-- Hosts are inserted before the parts attached to them.
INSERT INTO bom_items (bom_id, ref, part_number, upstream_ref, interface) VALUES
    (1, 'U1', 'EX-MCU32',   NULL, NULL),
    (1, 'U4', 'EX-I2CMUX8', 'U1', 'I2C'),
    (1, 'U2', 'EX-TEMP33',  'U4', 'I2C'),
    (1, 'U3', 'EX-DISP50',  'U1', 'SPI'),
    (1, 'C1', 'USER-LED2',  'U1', 'GPIO');
