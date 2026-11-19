// SYS-02 / SYS-04: invalid or unlikely specification values are flagged for manual review.
#include <gtest/gtest.h>

#include <limits>
#include <string>
#include <vector>

#include "TestParts.hpp"
#include "embedded_market/rules/SpecSanityRule.hpp"

using namespace em_test;

namespace {

std::vector<Finding> evaluate(const PartBuilder& part, EngineSettings settings = {}) {
    Project project(supply(3.3));
    project.addComponent(part.build());
    return SpecSanityRule().evaluate(project.components().front(), EvaluationContext(project, settings));
}

// Expects exactly one Warning naming U1 whose message contains `text`.
void expectSingleWarning(const std::vector<Finding>& findings, const std::string& text) {
    ASSERT_EQ(findings.size(), 1u);
    EXPECT_EQ(findings[0].severity, Severity::Warning);
    EXPECT_TRUE(findings[0].involves("U1"));
    EXPECT_NE(findings[0].message.find(text), std::string::npos) << findings[0].message;
}

}  // namespace

TEST(SpecSanityRuleTest, PlausibleSpecsPass) {
    const auto findings = evaluate(sensor3v3("U1", {Interface::I2C}));
    ASSERT_EQ(findings.size(), 1u);
    EXPECT_EQ(findings[0].severity, Severity::Pass);
}

TEST(SpecSanityRuleTest, InvertedSupplyRangeIsWarning) {
    expectSingleWarning(evaluate(sensor3v3("U1", {Interface::I2C}).supply(3.6, 3.4).logic(3.3, 2.0, 3.6)),
                        "minimum supply voltage (3.6 V) exceeds maximum supply voltage (3.4 V)");
}

TEST(SpecSanityRuleTest, NegativeVoltageIsWarning) {
    PartBuilder part = sensor3v3("U1", {Interface::I2C});
    part.specs().supplyVoltageMin = SpecValue<double>::verified(-1.0);
    expectSingleWarning(evaluate(part), "minimum supply voltage is negative");
}

TEST(SpecSanityRuleTest, NonFiniteVoltageIsWarning) {
    PartBuilder part = sensor3v3("U1", {Interface::I2C});
    part.specs().logicInputMax = SpecValue<double>::verified(std::numeric_limits<double>::quiet_NaN());
    expectSingleWarning(evaluate(part), "maximum input voltage is not a finite number");
}

TEST(SpecSanityRuleTest, ImplausiblyHighVoltageIsWarning) {
    PartBuilder part = sensor3v3("U1", {Interface::I2C});
    part.specs().logicInputMax = SpecValue<double>::verified(120.0);
    expectSingleWarning(evaluate(part), "implausibly high");
}

TEST(SpecSanityRuleTest, OutputHighAboveSupplyIsWarning) {
    PartBuilder part = sensor3v3("U1", {Interface::I2C});
    part.specs().logicOutputHigh = SpecValue<double>::verified(5.0);
    expectSingleWarning(evaluate(part), "logic output-high voltage (5 V) exceeds maximum supply voltage");
}

TEST(SpecSanityRuleTest, NegativePinCountIsWarning) {
    expectSingleWarning(evaluate(sensor3v3("U1", {Interface::I2C}).extraPins(-2)), "extra pin count is negative");
}

TEST(SpecSanityRuleTest, ImplausibleGpioCountIsWarning) {
    expectSingleWarning(evaluate(sensor3v3("U1", {Interface::I2C}).gpio(10000)),
                        "available GPIO count of 10000 is implausibly high");
}

TEST(SpecSanityRuleTest, LimitsAreConfigurable) {
    EngineSettings strict;
    strict.limits.maxPlausibleVoltage = 3.0;
    const auto findings = evaluate(sensor3v3("U1", {Interface::I2C}), strict);
    EXPECT_FALSE(findings.empty());
    for (const Finding& f : findings) EXPECT_EQ(f.severity, Severity::Warning);
}

TEST(SpecSanityRuleTest, UnverifiedValuesAreStillChecked) {
    PartBuilder part = sensor3v3("U1", {Interface::I2C});
    part.specs().supplyVoltageMin = SpecValue<double>::unverified(-3.3);
    expectSingleWarning(evaluate(part), "negative");
}

TEST(SpecSanityRuleTest, MissingValuesAreLeftToOtherRules) {
    PartBuilder part("U1", ComponentRole::Peripheral);
    const auto findings = evaluate(part);
    ASSERT_EQ(findings.size(), 1u);
    EXPECT_EQ(findings[0].severity, Severity::Pass);
}
