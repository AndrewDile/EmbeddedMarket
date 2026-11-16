// FR-06 / SYS-03: pin availability, counted by interface.
#include <gtest/gtest.h>

#include <string>

#include "TestParts.hpp"
#include "embedded_market/engine/Report.hpp"
#include "embedded_market/rules/PinBudgetRule.hpp"

using namespace em_test;

namespace {

Report evaluate(const Project& project) {
    const EngineSettings settings;
    Report report;
    for (Finding& f : PinBudgetRule().evaluate(EvaluationContext(project, settings))) report.add(std::move(f));
    return report;
}

// MCU1 with `gpio` pins hosting `count` sensors on `iface`, each needing `extra` extra pins.
Project hostWith(int gpio, Interface iface, int count, int extra = 0) {
    Project project(supply(3.3));
    PartBuilder host = mcu3v3();
    host.interfaces({Interface::I2C, Interface::SPI, Interface::UART}).gpio(gpio);
    project.addComponent(host.build());
    for (int i = 1; i <= count; ++i) {
        const std::string id = "U" + std::to_string(i);
        project.addComponent(sensor3v3(id, {iface}).extraPins(extra).build());
        project.addConnection(Connection("MCU1", id, iface));
    }
    return project;
}

Severity statusOf(const Project& project) { return evaluate(project).status(); }

}  // namespace

// I2C devices share SDA/SCL: three sensors need 2 pins, not 6.
TEST(PinBudgetRuleTest, I2CDevicesShareBusPins) {
    EXPECT_EQ(statusOf(hostWith(2, Interface::I2C, 3)), Severity::Pass);
    EXPECT_EQ(statusOf(hostWith(1, Interface::I2C, 3)), Severity::Conflict);
}

// SPI shares SCK/MOSI/MISO but needs one chip-select per device: 3 + 2 = 5.
TEST(PinBudgetRuleTest, SpiNeedsChipSelectPerDevice) {
    EXPECT_EQ(statusOf(hostWith(5, Interface::SPI, 2)), Severity::Pass);
    EXPECT_EQ(statusOf(hostWith(4, Interface::SPI, 2)), Severity::Conflict);
}

// UART is point-to-point: TX/RX per device.
TEST(PinBudgetRuleTest, UartPinsArePerDevice) {
    EXPECT_EQ(statusOf(hostWith(4, Interface::UART, 2)), Severity::Pass);
    EXPECT_EQ(statusOf(hostWith(3, Interface::UART, 2)), Severity::Conflict);
}

TEST(PinBudgetRuleTest, ExtraPinsAreAddedPerPart) {
    // Two I2C sensors with an interrupt line each: 2 + 1 + 1 = 4.
    EXPECT_EQ(statusOf(hostWith(4, Interface::I2C, 2, 1)), Severity::Pass);
    EXPECT_EQ(statusOf(hostWith(3, Interface::I2C, 2, 1)), Severity::Conflict);
}

TEST(PinBudgetRuleTest, MixedInterfacesAddUp) {
    Project project = hostWith(6, Interface::I2C, 1);  // I2C bus: 2
    project.addComponent(sensor3v3("S1", {Interface::SPI}).build());
    project.addConnection(Connection("MCU1", "S1", Interface::SPI));  // SPI bus 3 + CS 1
    EXPECT_EQ(statusOf(project), Severity::Pass);

    Project tight = hostWith(5, Interface::I2C, 1);
    tight.addComponent(sensor3v3("S1", {Interface::SPI}).build());
    tight.addConnection(Connection("MCU1", "S1", Interface::SPI));
    EXPECT_EQ(statusOf(tight), Severity::Conflict);
}

TEST(PinBudgetRuleTest, ConflictNamesHostAndExplainsBreakdown) {
    const Report report = evaluate(hostWith(4, Interface::SPI, 2));
    ASSERT_EQ(report.findings().size(), 1u);
    const Finding& f = report.findings()[0];
    EXPECT_EQ(f.severity, Severity::Conflict);
    EXPECT_EQ(f.componentIds.front(), "MCU1");
    EXPECT_NE(f.message.find("need 5 pins (SPI bus 3, U1 SPI 1, U2 SPI 1) but only 4 GPIO"), std::string::npos)
        << f.message;
}

TEST(PinBudgetRuleTest, ComponentsWithoutDownstreamPartsAreSkipped) {
    Project project(supply(3.3));
    project.addComponent(mcu3v3().build());
    EXPECT_TRUE(evaluate(project).findings().empty());
}

TEST(PinBudgetRuleTest, RepeatedLinksAreCountedOnce) {
    Project project = hostWith(4, Interface::SPI, 1);  // 3 + 1 = 4
    project.addConnection(Connection("MCU1", "U1", Interface::SPI));
    EXPECT_EQ(statusOf(project), Severity::Pass);
}

TEST(PinBudgetRuleTest, EachHostHasItsOwnBudget) {
    Project project(supply(3.3));
    project.addComponent(mcu3v3("MCU1").gpio(2).build());
    project.addComponent(mcu3v3("MCU2").gpio(1).build());
    project.addComponent(sensor3v3("U1", {Interface::I2C}).build());
    project.addComponent(sensor3v3("U2", {Interface::I2C}).build());
    project.addConnection(Connection("MCU1", "U1", Interface::I2C));
    project.addConnection(Connection("MCU2", "U2", Interface::I2C));

    const Report report = evaluate(project);
    ASSERT_EQ(report.findings().size(), 2u);
    ASSERT_EQ(report.withSeverity(Severity::Conflict).size(), 1u);
    EXPECT_EQ(report.withSeverity(Severity::Conflict)[0].componentIds.front(), "MCU2");
}

// MCU -> motor driver (SPI) -> motor (GPIO step/dir/enable/fault): each host has its own budget.
TEST(PinBudgetRuleTest, NestedHostsAreBudgetedIndependently) {
    Project project(supply(3.3));
    project.addComponent(mcu3v3("MCU1").gpio(4).build());
    project.addComponent(PartBuilder("DRV1", ComponentRole::Peripheral).extraPins(0).gpio(2).build());
    project.addComponent(PartBuilder("M1", ComponentRole::Peripheral).extraPins(4).build());
    project.addConnection(Connection("MCU1", "DRV1", Interface::SPI));  // 3 + 1 = 4 of 4
    project.addConnection(Connection("DRV1", "M1", Interface::GPIO));   // 4 of 2

    const Report report = evaluate(project);
    ASSERT_EQ(report.findings().size(), 2u);
    ASSERT_EQ(report.withSeverity(Severity::Conflict).size(), 1u);
    EXPECT_EQ(report.withSeverity(Severity::Conflict)[0].componentIds.front(), "DRV1");
}

TEST(PinBudgetRuleTest, MissingGpioCountIsWarning) {
    Project project(supply(3.3));
    PartBuilder host = mcu3v3();
    host.specs().availableGpio = SpecValue<int>::missing();
    project.addComponent(host.build());
    project.addComponent(sensor3v3("U1", {Interface::I2C}).build());
    project.addConnection(Connection("MCU1", "U1", Interface::I2C));
    EXPECT_EQ(statusOf(project), Severity::Warning);
}
