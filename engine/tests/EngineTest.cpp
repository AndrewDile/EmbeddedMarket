// SYS-03: the engine runs every registered rule and is extensible with new rules.
#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

#include "TestParts.hpp"
#include "embedded_market/engine/CompatibilityEngine.hpp"
#include "embedded_market/rules/SupplyVoltageRule.hpp"

using namespace em_test;

namespace {

// Example of a custom rule: flags any component whose id is "BAD".
class NoBadIdsRule : public ComponentRule {
public:
    std::string id() const override { return "no-bad-ids"; }
    std::string description() const override { return "Test rule."; }
    std::vector<Finding> evaluate(const Component& component, const EvaluationContext&) const override {
        if (component.id() == "BAD") return {{Severity::Conflict, id(), {component.id()}, "bad id"}};
        return {};
    }
};

class ThrowingRule : public ProjectRule {
public:
    std::string id() const override { return "throws"; }
    std::string description() const override { return "Test rule."; }
    std::vector<Finding> evaluate(const EvaluationContext&) const override {
        throw std::runtime_error("boom");
    }
};

Project compatibleProject() {
    Project project(supply(3.3));
    project.addComponent(mcu3v3("MCU1").build());
    project.addComponent(sensor3v3("U1", {Interface::I2C}).build());
    project.addConnection(Connection("MCU1", "U1", Interface::I2C));
    return project;
}

}  // namespace

TEST(EngineTest, DefaultRegistryHasBuiltInRules) {
    const RuleRegistry registry = RuleRegistry::withDefaults();
    EXPECT_EQ(registry.size(), 5u);
    for (const char* id : {"spec-sanity", "supply-voltage", "interface", "logic-level", "pin-budget"}) {
        EXPECT_TRUE(registry.contains(id)) << id;
    }
}

TEST(EngineTest, EmptyProjectPasses) {
    CompatibilityEngine engine;
    const Report report = engine.evaluate(Project());
    EXPECT_EQ(report.status(), Severity::Pass);
    EXPECT_TRUE(report.findings().empty());
}

TEST(EngineTest, CompatibleProjectPassesWithExplanations) {
    CompatibilityEngine engine;
    const Report report = engine.evaluate(compatibleProject());
    EXPECT_EQ(report.status(), Severity::Pass);
    // Every built-in rule explains its pass.
    for (const char* id : {"spec-sanity", "supply-voltage", "interface", "logic-level", "pin-budget"}) {
        EXPECT_FALSE(report.fromRule(id).empty()) << id;
    }
    for (const Finding& finding : report.findings()) {
        EXPECT_FALSE(finding.message.empty());
    }
}

TEST(EngineTest, CustomRulesRunAlongsideBuiltIns) {
    RuleRegistry registry = RuleRegistry::withDefaults();
    registry.emplace<NoBadIdsRule>();
    CompatibilityEngine engine(std::move(registry));

    Project project = compatibleProject();
    project.addComponent(sensor3v3("BAD", {Interface::I2C}).build());
    const Report report = engine.evaluate(project);

    EXPECT_EQ(report.status(), Severity::Conflict);
    ASSERT_EQ(report.fromRule("no-bad-ids").size(), 1u);
    EXPECT_TRUE(report.fromRule("no-bad-ids")[0].involves("BAD"));
}

TEST(EngineTest, OnlyRegisteredRulesRun) {
    RuleRegistry registry;
    registry.emplace<SupplyVoltageRule>();
    CompatibilityEngine engine(std::move(registry));

    const Report report = engine.evaluate(compatibleProject());
    EXPECT_EQ(report.findings().size(), 2u);  // One per component.
    EXPECT_EQ(report.fromRule("supply-voltage").size(), 2u);
}

TEST(EngineTest, RejectsDuplicateRuleIds) {
    RuleRegistry registry;
    registry.emplace<SupplyVoltageRule>();
    EXPECT_THROW(registry.emplace<SupplyVoltageRule>(), std::invalid_argument);
    EXPECT_THROW(registry.add(std::unique_ptr<ComponentRule>()), std::invalid_argument);
}

TEST(EngineTest, ThrowingRuleBecomesWarning) {
    RuleRegistry registry;
    registry.emplace<ThrowingRule>();
    registry.emplace<SupplyVoltageRule>();
    CompatibilityEngine engine(std::move(registry));

    const Report report = engine.evaluate(compatibleProject());
    EXPECT_EQ(report.status(), Severity::Warning);
    ASSERT_EQ(report.fromRule("throws").size(), 1u);
    EXPECT_NE(report.fromRule("throws")[0].message.find("boom"), std::string::npos);
    EXPECT_EQ(report.fromRule("supply-voltage").size(), 2u);  // Other rules still ran.
}
