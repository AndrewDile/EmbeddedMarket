// FR-03 / OBJ3 / SYS-03: communication interface compatibility. The controller (mcu3v3)
// supports I2C and SPI, per the FR-03 test method.
#include <gtest/gtest.h>

#include <vector>

#include "TestParts.hpp"
#include "embedded_market/rules/InterfaceRule.hpp"

using namespace em_test;

namespace {

std::vector<Finding> evaluateLink(const PartBuilder& host, const PartBuilder& part, Interface chosen) {
    Project project(supply(3.3));
    project.addComponent(host.build());
    project.addComponent(part.build());
    const Connection connection(project.components()[0].id(), project.components()[1].id(), chosen);
    const EngineSettings settings;
    return InterfaceRule().evaluate(connection, project.components()[0], project.components()[1],
                                    EvaluationContext(project, settings));
}

}  // namespace

// OBJ3 case 1: matching I2C.
TEST(InterfaceRuleObj3Test, MatchingI2CPasses) {
    const auto findings = evaluateLink(mcu3v3(), sensor3v3("U1", {Interface::I2C}), Interface::I2C);
    ASSERT_EQ(findings.size(), 1u);
    EXPECT_EQ(findings[0].severity, Severity::Pass);
}

// OBJ3 case 2: matching SPI.
TEST(InterfaceRuleObj3Test, MatchingSPIPasses) {
    const auto findings = evaluateLink(mcu3v3(), sensor3v3("U1", {Interface::SPI}), Interface::SPI);
    ASSERT_EQ(findings.size(), 1u);
    EXPECT_EQ(findings[0].severity, Severity::Pass);
}

// OBJ3 case 3: SPI-only peripheral connected over I2C.
TEST(InterfaceRuleObj3Test, SpiOnlyPeripheralOnI2CConflicts) {
    const auto findings = evaluateLink(mcu3v3(), sensor3v3("U1", {Interface::SPI}), Interface::I2C);
    ASSERT_EQ(findings.size(), 1u);
    EXPECT_EQ(findings[0].severity, Severity::Conflict);
    EXPECT_EQ(findings[0].componentIds, std::vector<std::string>{"U1"});
    EXPECT_NE(findings[0].message.find("does not support I2C"), std::string::npos) << findings[0].message;
}

// OBJ3 case 4: UART-only peripheral connected over SPI.
TEST(InterfaceRuleObj3Test, UartOnlyPeripheralOnSPIConflicts) {
    const auto findings = evaluateLink(mcu3v3(), sensor3v3("U1", {Interface::UART}), Interface::SPI);
    ASSERT_EQ(findings.size(), 1u);
    EXPECT_EQ(findings[0].severity, Severity::Conflict);
    EXPECT_EQ(findings[0].componentIds, std::vector<std::string>{"U1"});
}

TEST(InterfaceRuleTest, HostMissingInterfaceIsBlamed) {
    PartBuilder host = mcu3v3();
    host.interfaces({Interface::I2C});
    const auto findings = evaluateLink(host, sensor3v3("U1", {Interface::SPI}), Interface::SPI);
    ASSERT_EQ(findings.size(), 1u);
    EXPECT_EQ(findings[0].severity, Severity::Conflict);
    EXPECT_EQ(findings[0].componentIds, std::vector<std::string>{"MCU1"});
}

TEST(InterfaceRuleTest, BothEndsMissingInterfaceReportsEach) {
    PartBuilder host = mcu3v3();
    host.interfaces({Interface::I2C});
    const auto findings = evaluateLink(host, sensor3v3("U1", {Interface::I2C}), Interface::UART);
    ASSERT_EQ(findings.size(), 2u);
    EXPECT_TRUE(findings[0].involves("MCU1"));
    EXPECT_TRUE(findings[1].involves("U1"));
}

TEST(InterfaceRuleTest, MissingInterfaceListIsWarning) {
    PartBuilder part = sensor3v3("U1", {Interface::I2C});
    part.specs().interfaces = SpecValue<InterfaceSet>::missing();
    const auto findings = evaluateLink(mcu3v3(), part, Interface::I2C);
    ASSERT_EQ(findings.size(), 1u);
    EXPECT_EQ(findings[0].severity, Severity::Warning);
    EXPECT_TRUE(findings[0].involves("U1"));
}
