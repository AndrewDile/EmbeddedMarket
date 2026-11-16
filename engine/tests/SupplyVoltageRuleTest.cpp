// FR-02 / OBJ2 / SYS-03: supply-voltage compatibility.
#include <gtest/gtest.h>

#include <ostream>
#include <string>
#include <vector>

#include "TestParts.hpp"
#include "embedded_market/rules/SupplyVoltageRule.hpp"

using namespace em_test;

namespace {

// The FR-02 part: operating range 1.71 V to 3.60 V.
PartBuilder fr02Part() { return std::move(PartBuilder("U1", ComponentRole::Peripheral).supply(1.71, 3.60)); }

std::vector<Finding> evaluateOn(SpecValue<VoltageRange> projectSupply, const PartBuilder& part = fr02Part()) {
    Project project(std::move(projectSupply));
    project.addComponent(part.build());
    const EngineSettings settings;
    return SupplyVoltageRule().evaluate(project.components().front(), EvaluationContext(project, settings));
}

struct SupplyCase {
    std::string name;
    double min;
    double max;
    Severity expected;
};

void PrintTo(const SupplyCase& c, std::ostream* os) {
    *os << c.name << " ([" << c.min << ", " << c.max << "] V -> " << c.expected << ")";
}

class SupplyVoltageFr02Test : public ::testing::TestWithParam<SupplyCase> {};

}  // namespace

// FR-02 / OBJ2 acceptance: all five declared supply ranges must match (100% agreement).
TEST_P(SupplyVoltageFr02Test, MatchesExpectedResult) {
    const SupplyCase& c = GetParam();
    const auto findings = evaluateOn(supply(c.min, c.max));
    ASSERT_EQ(findings.size(), 1u);
    EXPECT_EQ(findings[0].severity, c.expected) << findings[0].message;
    EXPECT_TRUE(findings[0].involves("U1"));
}

INSTANTIATE_TEST_SUITE_P(Fr02, SupplyVoltageFr02Test,
                         ::testing::Values(SupplyCase{"Fixed3V3", 3.30, 3.30, Severity::Pass},
                                           SupplyCase{"ExactlyOperatingRange", 1.71, 3.60, Severity::Pass},
                                           SupplyCase{"DipsBelowMinimum", 1.60, 3.30, Severity::Conflict},
                                           SupplyCase{"RisesAboveMaximum", 3.30, 3.70, Severity::Conflict},
                                           SupplyCase{"Fixed5V", 5.0, 5.0, Severity::Conflict}),
                         [](const auto& info) { return info.param.name; });

TEST(SupplyVoltageRuleTest, ToleranceAbsorbsFloatingPointFuzz) {
    EXPECT_EQ(evaluateOn(supply(1.71 - 5e-7, 3.60 + 5e-7))[0].severity, Severity::Pass);
}

TEST(SupplyVoltageRuleTest, ToleranceIsNotAnEngineeringMargin) {
    EXPECT_EQ(evaluateOn(supply(1.71, 3.60 + 1e-5))[0].severity, Severity::Conflict);
    EXPECT_EQ(evaluateOn(supply(1.71 - 1e-5, 3.60))[0].severity, Severity::Conflict);
}

TEST(SupplyVoltageRuleTest, OverlapAloneIsNotCompatibility) {
    // [1.0, 5.0] overlaps [1.71, 3.60] but extends past both ends.
    const auto findings = evaluateOn(supply(1.0, 5.0));
    ASSERT_EQ(findings.size(), 1u);
    EXPECT_EQ(findings[0].severity, Severity::Conflict);
    EXPECT_NE(findings[0].message.find("below its minimum"), std::string::npos) << findings[0].message;
    EXPECT_NE(findings[0].message.find("above its maximum"), std::string::npos) << findings[0].message;
}

TEST(SupplyVoltageRuleTest, ConflictExplainsWhichLimitWasViolated) {
    const auto over = evaluateOn(supply(3.30, 3.70));
    EXPECT_NE(over[0].message.find("above its maximum operating voltage of 3.6 V"), std::string::npos)
        << over[0].message;
    const auto under = evaluateOn(supply(1.60, 3.30));
    EXPECT_NE(under[0].message.find("below its minimum operating voltage of 1.71 V"), std::string::npos)
        << under[0].message;
}

TEST(SupplyVoltageRuleTest, MissingProjectSupplyIsWarning) {
    const auto findings = evaluateOn(SpecValue<VoltageRange>::missing());
    ASSERT_EQ(findings.size(), 1u);
    EXPECT_EQ(findings[0].severity, Severity::Warning);
}

TEST(SupplyVoltageRuleTest, InvertedProjectSupplyIsWarning) {
    const auto findings = evaluateOn(supply(3.6, 1.8));
    ASSERT_EQ(findings.size(), 1u);
    EXPECT_EQ(findings[0].severity, Severity::Warning);
    EXPECT_NE(findings[0].message.find("invalid value"), std::string::npos) << findings[0].message;
}

TEST(SupplyVoltageRuleTest, InvertedOperatingRangeIsWarning) {
    PartBuilder part("U1", ComponentRole::Peripheral);
    part.supply(3.6, 1.8);
    const auto findings = evaluateOn(supply(3.3), part);
    ASSERT_EQ(findings.size(), 1u);
    EXPECT_EQ(findings[0].severity, Severity::Warning);
}

TEST(SupplyVoltageRuleTest, UnverifiedInRangeIsWarningNotPass) {
    PartBuilder part = fr02Part();
    part.specs().supplyVoltageMax = SpecValue<double>::unverified(3.60);
    const auto findings = evaluateOn(supply(3.3), part);
    ASSERT_EQ(findings.size(), 1u);
    EXPECT_EQ(findings[0].severity, Severity::Warning);
    EXPECT_NE(findings[0].message.find("unverified"), std::string::npos);
}

TEST(SupplyVoltageRuleTest, UnverifiedOutOfRangeIsStillConflict) {
    PartBuilder part = fr02Part();
    part.specs().supplyVoltageMax = SpecValue<double>::unverified(3.60);
    const auto findings = evaluateOn(supply(5.0), part);
    ASSERT_EQ(findings.size(), 1u);
    EXPECT_EQ(findings[0].severity, Severity::Conflict);
    EXPECT_NE(findings[0].message.find("unverified"), std::string::npos);
}
