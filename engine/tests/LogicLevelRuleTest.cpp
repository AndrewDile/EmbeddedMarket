// SYS-03: logic-level compatibility between connected parts.
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "TestParts.hpp"
#include "embedded_market/engine/Report.hpp"
#include "embedded_market/rules/LogicLevelRule.hpp"

using namespace em_test;

namespace {

Report evaluateLink(const PartBuilder& host, const PartBuilder& part) {
    Project project(supply(3.3));
    project.addComponent(host.build());
    project.addComponent(part.build());
    const Connection connection(project.components()[0].id(), project.components()[1].id(), Interface::I2C);
    const EngineSettings settings;
    Report report;
    for (Finding& f : LogicLevelRule().evaluate(connection, project.components()[0], project.components()[1],
                                                EvaluationContext(project, settings))) {
        report.add(std::move(f));
    }
    return report;
}

}  // namespace

TEST(LogicLevelRuleTest, MatchingLevelsPassInBothDirections) {
    const Report report = evaluateLink(mcu3v3(), sensor3v3("U1", {Interface::I2C}));
    EXPECT_EQ(report.status(), Severity::Pass);
    EXPECT_EQ(report.findings().size(), 2u);
}

TEST(LogicLevelRuleTest, ThreeVoltDriverIntoFiveVoltTolerantInputPasses) {
    PartBuilder part = sensor3v3("U1", {Interface::I2C});
    part.logic(3.3, 2.0, 5.5);
    EXPECT_EQ(evaluateLink(mcu3v3(), part).status(), Severity::Pass);
}

TEST(LogicLevelRuleTest, FiveVoltDriverIntoThreeVoltInputConflicts) {
    PartBuilder part = sensor3v3("U1", {Interface::I2C});
    part.logic(5.0, 3.5, 5.5);  // 5 V part: drives 5 V, needs >= 3.5 V for a high.
    const Report report = evaluateLink(mcu3v3(), part);

    EXPECT_EQ(report.status(), Severity::Conflict);
    const auto conflicts = report.withSeverity(Severity::Conflict);
    ASSERT_EQ(conflicts.size(), 2u);  // Overvoltage into MCU1, and MCU1's 3.3 V high too low for U1.

    bool sawDamageRisk = false;
    bool sawThreshold = false;
    for (const Finding& f : conflicts) {
        EXPECT_TRUE(f.involves("MCU1") && f.involves("U1"));
        sawDamageRisk |= f.message.find("risk of damage") != std::string::npos;
        sawThreshold |= f.message.find("may not register") != std::string::npos;
    }
    EXPECT_TRUE(sawDamageRisk);
    EXPECT_TRUE(sawThreshold);
}

TEST(LogicLevelRuleTest, OutputHighBelowInputThresholdConflicts) {
    PartBuilder part = sensor3v3("U1", {Interface::I2C});
    part.logic(3.3, 3.5, 5.5);
    const Report report = evaluateLink(mcu3v3(), part);
    ASSERT_EQ(report.withSeverity(Severity::Conflict).size(), 1u);
    EXPECT_EQ(report.withSeverity(Severity::Conflict)[0].componentIds, (std::vector<std::string>{"MCU1", "U1"}));
}

TEST(LogicLevelRuleTest, MissingInputLimitIsWarning) {
    PartBuilder part = sensor3v3("U1", {Interface::I2C});
    part.specs().logicInputMax = SpecValue<double>::missing();
    const Report report = evaluateLink(mcu3v3(), part);
    EXPECT_EQ(report.status(), Severity::Warning);
    ASSERT_EQ(report.withSeverity(Severity::Warning).size(), 1u);
    EXPECT_TRUE(report.withSeverity(Severity::Warning)[0].involves("U1"));
}

TEST(LogicLevelRuleTest, UnverifiedOutputIsWarning) {
    PartBuilder part = sensor3v3("U1", {Interface::I2C});
    part.specs().logicOutputHigh = SpecValue<double>::unverified(3.3);
    const Report report = evaluateLink(mcu3v3(), part);
    EXPECT_EQ(report.status(), Severity::Warning);
}
