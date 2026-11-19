// The Field Node demo flow: BOM -> failed check -> revise -> recheck -> approve.
// Part values match the UI's demo BOM and the database seed.
#include <gtest/gtest.h>

#include <optional>
#include <string>

#include "TestParts.hpp"
#include "embedded_market/engine/CompatibilityEngine.hpp"

using namespace em_test;

namespace {

PartBuilder fieldPart(std::string id, InterfaceSet interfaces, double outputHigh, double supplyMax, double inputMax,
                      int extraPins, std::optional<double> currentMa) {
    PartBuilder part(std::move(id), ComponentRole::Peripheral);
    part.supply(3.0, supplyMax).interfaces(std::move(interfaces)).logic(outputHigh, 2.0, inputMax).extraPins(extraPins);
    if (currentMa) part.specs().currentMa = SpecValue<double>::verified(*currentMa);
    return part;
}

Project fieldNode(bool fiveVoltDisplay, std::optional<double> ledCurrentMa) {
    ProjectConstraints constraints;
    constraints.powerBudgetMa = 200.0;
    constraints.allowedInterfaces = InterfaceSet{Interface::I2C, Interface::SPI, Interface::GPIO};
    constraints.minClockMhz = 48.0;
    constraints.minFlashKb = 256.0;
    constraints.minRamKb = 64.0;
    Project project(supply(3.3), constraints);

    PartBuilder mcu("U1", ComponentRole::Controller);
    mcu.supply(3.0, 3.6)
        .interfaces({Interface::I2C, Interface::SPI, Interface::UART, Interface::GPIO})
        .logic(3.3, 2.0, 3.3)
        .gpio(20)
        .extraPins(0);
    mcu.specs().currentMa = SpecValue<double>::verified(40.0);
    mcu.specs().clockMhz = SpecValue<double>::verified(48.0);
    mcu.specs().flashKb = SpecValue<double>::verified(256.0);
    mcu.specs().ramKb = SpecValue<double>::verified(64.0);
    project.addComponent(mcu.build());

    PartBuilder mux = fieldPart("U4", {Interface::I2C}, 3.3, 3.6, 3.6, 0, 2.0);
    mux.gpio(16);
    project.addComponent(mux.build());
    project.addComponent(fieldPart("U2", {Interface::I2C}, 3.3, 3.6, 3.6, 0, 10.0).build());
    project.addComponent(fiveVoltDisplay ? fieldPart("U3", {Interface::SPI}, 5.0, 5.5, 5.5, 1, 80.0).build()
                                         : fieldPart("U3", {Interface::SPI}, 3.3, 3.6, 3.6, 1, 50.0).build());
    project.addComponent(fieldPart("C1", {Interface::GPIO}, 3.3, 3.6, 3.6, 2, ledCurrentMa).build());

    project.addConnection(Connection("U1", "U4", Interface::I2C));
    project.addConnection(Connection("U4", "U2", Interface::I2C));
    project.addConnection(Connection("U1", "U3", Interface::SPI));
    project.addConnection(Connection("U1", "C1", Interface::GPIO));
    return project;
}

Report check(bool fiveVoltDisplay, std::optional<double> ledCurrentMa) {
    return CompatibilityEngine().evaluate(fieldNode(fiveVoltDisplay, ledCurrentMa));
}

double metric(const Report& report, const std::string& ruleId, const std::string& name) {
    for (const Finding& finding : report.fromRule(ruleId)) {
        const auto it = finding.metrics.find(name);
        if (it != finding.metrics.end()) return it->second;
    }
    ADD_FAILURE() << "No " << name << " metric from " << ruleId;
    return -1.0;
}

}  // namespace

// Results page: E01 display logic conflict, W01 missing C1 current, W02 incomplete power budget.
TEST(FieldNodeFlowTest, FirstRunHasOneConflictAndTwoWarnings) {
    const Report report = check(true, std::nullopt);
    EXPECT_EQ(report.status(), Severity::Conflict);

    const auto conflicts = report.withSeverity(Severity::Conflict);
    ASSERT_EQ(conflicts.size(), 1u);
    EXPECT_EQ(conflicts[0].ruleId, "logic-level");
    EXPECT_TRUE(conflicts[0].involves("U3"));
    EXPECT_TRUE(conflicts[0].involves("U1"));
    EXPECT_DOUBLE_EQ(conflicts[0].metrics.at("outputHighV"), 5.0);
    EXPECT_DOUBLE_EQ(conflicts[0].metrics.at("inputMaxV"), 3.3);

    const auto warnings = report.withSeverity(Severity::Warning);
    ASSERT_EQ(warnings.size(), 2u);
    EXPECT_TRUE(warnings[0].involves("C1"));
    EXPECT_EQ(warnings[1].ruleId, "power-budget");
    EXPECT_DOUBLE_EQ(metric(report, "power-budget", "knownMa"), 132.0);
    EXPECT_DOUBLE_EQ(metric(report, "power-budget", "unknownParts"), 1.0);
}

TEST(FieldNodeFlowTest, ReplacingOnlyTheDisplayLeavesTheCurrentWarnings) {
    const Report report = check(false, std::nullopt);
    EXPECT_EQ(report.status(), Severity::Warning);
    EXPECT_TRUE(report.withSeverity(Severity::Conflict).empty());
    EXPECT_EQ(report.withSeverity(Severity::Warning).size(), 2u);
}

TEST(FieldNodeFlowTest, EnteringOnlyTheCurrentLeavesTheDisplayConflict) {
    const Report report = check(true, 15.0);
    EXPECT_EQ(report.status(), Severity::Conflict);
    EXPECT_EQ(report.withSeverity(Severity::Conflict).size(), 1u);
    EXPECT_TRUE(report.withSeverity(Severity::Warning).empty());
}

// Finalized BOM page: 40 + 2 + 10 + 50 + 15 = 117 mA and 9 of 20 MCU pins.
TEST(FieldNodeFlowTest, RevisedBomPassesEveryCheck) {
    const Report report = check(false, 15.0);
    EXPECT_EQ(report.status(), Severity::Pass);
    EXPECT_DOUBLE_EQ(metric(report, "power-budget", "knownMa"), 117.0);
    EXPECT_DOUBLE_EQ(metric(report, "power-budget", "unknownParts"), 0.0);
    EXPECT_DOUBLE_EQ(metric(report, "pin-budget", "pinsUsed"), 9.0);
    EXPECT_DOUBLE_EQ(metric(report, "pin-budget", "pinsAvailable"), 20.0);
}

// The UI's E02: 102 mA known + 99 mA entered for C1 = 201 mA.
TEST(FieldNodeFlowTest, EnteredCurrentCanExceedTheBudget) {
    const Report report = check(false, 99.0);
    EXPECT_EQ(report.status(), Severity::Conflict);
    const auto conflicts = report.withSeverity(Severity::Conflict);
    ASSERT_EQ(conflicts.size(), 1u);
    EXPECT_EQ(conflicts[0].ruleId, "power-budget");
    EXPECT_DOUBLE_EQ(conflicts[0].metrics.at("knownMa"), 201.0);
}

TEST(FieldNodeFlowTest, CurrentExactlyAtTheBudgetPasses) {
    EXPECT_EQ(check(false, 98.0).status(), Severity::Pass);
}
