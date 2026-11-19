-- Embedded Market schema. MySQL runs this once, when the database volume is first created.
-- A NULL spec means the value is missing, which the rules engine reports for manual review.

CREATE TABLE parts (
    part_number            VARCHAR(40)  PRIMARY KEY,
    name                   VARCHAR(80)  NOT NULL,
    role                   ENUM('controller','peripheral') NOT NULL,
    supply_min_v           DECIMAL(5,2) NULL,
    supply_max_v           DECIMAL(5,2) NULL,
    logic_output_high_v    DECIMAL(5,2) NULL,
    logic_input_high_min_v DECIMAL(5,2) NULL,
    logic_input_max_v      DECIMAL(5,2) NULL,
    interfaces             SET('I2C','SPI','UART','GPIO') NULL,
    available_gpio         INT          NULL,
    extra_pins             INT          NULL,
    current_ma             DECIMAL(7,2) NULL,
    clock_mhz              DECIMAL(7,2) NULL,
    flash_kb               INT          NULL,
    ram_kb                 INT          NULL,
    CONSTRAINT chk_supply_range  CHECK (supply_min_v IS NULL OR supply_max_v IS NULL OR supply_min_v <= supply_max_v),
    CONSTRAINT chk_supply_values CHECK ((supply_min_v IS NULL OR supply_min_v >= 0) AND (supply_max_v IS NULL OR supply_max_v <= 60)),
    CONSTRAINT chk_logic_values  CHECK ((logic_output_high_v IS NULL OR logic_output_high_v >= 0)
                                    AND (logic_input_high_min_v IS NULL OR logic_input_high_min_v >= 0)
                                    AND (logic_input_max_v IS NULL OR logic_input_max_v >= 0)),
    CONSTRAINT chk_pin_counts    CHECK ((available_gpio IS NULL OR available_gpio >= 0) AND (extra_pins IS NULL OR extra_pins >= 0)),
    CONSTRAINT chk_current       CHECK (current_ma IS NULL OR current_ma >= 0)
);

CREATE TABLE boms (
    id                 INT AUTO_INCREMENT PRIMARY KEY,
    name               VARCHAR(80)  NOT NULL,
    supply_min_v       DECIMAL(5,2) NOT NULL,
    supply_max_v       DECIMAL(5,2) NOT NULL,
    power_budget_ma    DECIMAL(7,2) NOT NULL,
    allowed_interfaces SET('I2C','SPI','UART','GPIO') NOT NULL,
    min_clock_mhz      DECIMAL(7,2) NOT NULL,
    min_flash_kb       INT          NOT NULL,
    min_ram_kb         INT          NOT NULL,
    CONSTRAINT chk_bom_supply CHECK (supply_min_v <= supply_max_v),
    CONSTRAINT chk_bom_budget CHECK (power_budget_ma > 0)
);

CREATE TABLE bom_items (
    bom_id       INT         NOT NULL,
    ref          VARCHAR(8)  NOT NULL,
    part_number  VARCHAR(40) NOT NULL,
    upstream_ref VARCHAR(8)  NULL,
    interface    ENUM('I2C','SPI','UART','GPIO') NULL,
    PRIMARY KEY (bom_id, ref),
    CONSTRAINT fk_item_bom  FOREIGN KEY (bom_id) REFERENCES boms (id) ON DELETE CASCADE,
    CONSTRAINT fk_item_part FOREIGN KEY (part_number) REFERENCES parts (part_number),
    CONSTRAINT fk_item_upstream FOREIGN KEY (bom_id, upstream_ref) REFERENCES bom_items (bom_id, ref)
);

-- One row per compatibility check, so each run can be audited later.
CREATE TABLE check_runs (
    id         INT AUTO_INCREMENT PRIMARY KEY,
    bom_id     INT       NOT NULL,
    revision   INT       NOT NULL,
    ready      BOOLEAN   NOT NULL,
    report     JSON      NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_run_bom FOREIGN KEY (bom_id) REFERENCES boms (id) ON DELETE CASCADE
);
