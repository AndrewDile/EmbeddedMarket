// SYS-03 / FR-04: project power budget, including parts with unknown current.
#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>

#include "TestParts.hpp"
#include "embedded_market/engine/Report.hpp"
#include "embedded_market/rules/PowerBudgetRule.hpp"

using namespace em_test;

namespace {

Report evaluate(const Project& project) {
    const EngineSettings settings;
    Report report;
    for (Finding& f : PowerBudgetRule().evaluate(EvaluationContext(project, settings))) report.add(std::move(f));
    return report;
}

// One part per entry; an empty entry has a missing current.
Project projectDrawing(const std::vector<std::optional<double>>& currents, std::optional<double> budget = 200.0) {
    ProjectConstraints constraints;
    constraints.powerBudgetMa = budget;
    Project project(supply(3.3), constraints);
    int index = 1;
    for (const std::optional<double>& current : currents) {
        PartBuilder part = sensor3v3("U" + std::to_string(index++), {Interface::I2C});
        if (current) part.specs().currentMa = SpecValue<double>::verified(*current);
        project.addComponent(part.build());
    }
    return project;
}

}  // namespace

TEST(PowerBudgetRuleTest, TotalWithinBudgetPasses) {
    const Report report = evaluate(projectDrawing({40.0, 10.0, 50.0, 15.0, 2.0}));
    EXPECT_EQ(report.status(), Severity::Pass);
    ASSERT_EQ(report.findings().size(), 1u);
    EXPECT_DOUBLE_EQ(report.findings()[0].metrics.at("knownMa"), 117.0);
}

TEST(PowerBudgetRuleTest, TotalAtBudgetPasses) {
    EXPECT_EQ(evaluate(projectDrawing({150.0, 50.0})).status(), Severity::Pass);
}

TEST(PowerBudgetRuleTest, TotalOverBudgetConflicts) {
    EXPECT_EQ(evaluate(projectDrawing({150.0, 50.5})).status(), Severity::Conflict);
}

// The flow's W01 and W02: the part is named, and the project total is reported as incomplete.
TEST(PowerBudgetRuleTest, UnknownCurrentWarnsForPartAndProject) {
    const Report report = evaluate(projectDrawing({40.0, 90.0, std::nullopt}));
    EXPECT_EQ(report.status(), Severity::Warning);
    ASSERT_EQ(report.findings().size(), 2u);
    EXPECT_TRUE(report.findings()[0].involves("U3"));
    EXPECT_NE(report.findings()[0].message.find("maximum current is missing"), std::string::npos);
    EXPECT_NE(report.findings()[1].message.find("known allocations total 130 mA"), std::string::npos);
}

TEST(PowerBudgetRuleTest, KnownSubtotalOverBudgetConflictsEvenWithUnknownParts) {
    EXPECT_EQ(evaluate(projectDrawing({250.0, std::nullopt})).status(), Severity::Conflict);
}

TEST(PowerBudgetRuleTest, UnverifiedCurrentNeverPasses) {
    Project project = projectDrawing({});
    PartBuilder part = sensor3v3("U1", {Interface::I2C});
    part.specs().currentMa = SpecValue<double>::unverified(10.0);
    project.addComponent(part.build());
    EXPECT_EQ(evaluate(project).status(), Severity::Warning);
}

TEST(PowerBudgetRuleTest, NegativeCurrentNeedsReview) {
    EXPECT_EQ(evaluate(projectDrawing({-5.0})).status(), Severity::Warning);
}

TEST(PowerBudgetRuleTest, NoBudgetSkipsTheCheck) {
    EXPECT_TRUE(evaluate(projectDrawing({1000.0}, std::nullopt)).findings().empty());
}
