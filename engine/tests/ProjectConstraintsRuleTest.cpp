// SYS-03: user-declared clock, memory, and allowed-interface constraints.
#include <gtest/gtest.h>

#include "TestParts.hpp"
#include "embedded_market/engine/Report.hpp"
#include "embedded_market/rules/ProjectConstraintsRule.hpp"

using namespace em_test;

namespace {

Report evaluate(const Project& project) {
    const EngineSettings settings;
    Report report;
    for (Finding& f : ProjectConstraintsRule().evaluate(EvaluationContext(project, settings))) report.add(std::move(f));
    return report;
}

ProjectConstraints minimums() {
    ProjectConstraints constraints;
    constraints.minClockMhz = 48.0;
    constraints.minFlashKb = 256.0;
    constraints.minRamKb = 64.0;
    return constraints;
}

PartBuilder controller(double clock, double flash, double ram) {
    PartBuilder part = mcu3v3();
    part.specs().clockMhz = SpecValue<double>::verified(clock);
    part.specs().flashKb = SpecValue<double>::verified(flash);
    part.specs().ramKb = SpecValue<double>::verified(ram);
    return part;
}

}  // namespace

TEST(ProjectConstraintsRuleTest, ControllerMeetingMinimumsPasses) {
    Project project(supply(3.3), minimums());
    project.addComponent(controller(48.0, 256.0, 64.0).build());
    const Report report = evaluate(project);
    EXPECT_EQ(report.status(), Severity::Pass);
    EXPECT_EQ(report.findings().size(), 3u);
}

TEST(ProjectConstraintsRuleTest, ControllerBelowAnyMinimumConflicts) {
    Project project(supply(3.3), minimums());
    project.addComponent(controller(48.0, 128.0, 64.0).build());
    const Report report = evaluate(project);
    EXPECT_EQ(report.status(), Severity::Conflict);
    ASSERT_EQ(report.withSeverity(Severity::Conflict).size(), 1u);
    EXPECT_NE(report.withSeverity(Severity::Conflict)[0].message.find("flash memory"), std::string::npos);
}

TEST(ProjectConstraintsRuleTest, MissingControllerSpecWarns) {
    Project project(supply(3.3), minimums());
    PartBuilder part = controller(48.0, 256.0, 64.0);
    part.specs().ramKb = SpecValue<double>::missing();
    project.addComponent(part.build());
    EXPECT_EQ(evaluate(project).status(), Severity::Warning);
}

TEST(ProjectConstraintsRuleTest, PeripheralsAreNotHeldToControllerMinimums) {
    Project project(supply(3.3), minimums());
    project.addComponent(sensor3v3("U1", {Interface::I2C}).build());
    EXPECT_TRUE(evaluate(project).findings().empty());
}

TEST(ProjectConstraintsRuleTest, DisallowedInterfaceConflicts) {
    ProjectConstraints constraints;
    constraints.allowedInterfaces = InterfaceSet{Interface::I2C, Interface::SPI, Interface::GPIO};
    Project project(supply(3.3), constraints);
    project.addComponent(mcu3v3().build());
    project.addComponent(sensor3v3("U1", {Interface::I2C}).build());
    project.addComponent(sensor3v3("U2", {Interface::UART}).build());
    project.addConnection(Connection("MCU1", "U1", Interface::I2C));
    project.addConnection(Connection("MCU1", "U2", Interface::UART));

    const Report report = evaluate(project);
    EXPECT_EQ(report.withSeverity(Severity::Pass).size(), 1u);
    ASSERT_EQ(report.withSeverity(Severity::Conflict).size(), 1u);
    EXPECT_TRUE(report.withSeverity(Severity::Conflict)[0].involves("U2"));
}

TEST(ProjectConstraintsRuleTest, UndeclaredConstraintsAreSkipped) {
    Project project(supply(3.3));
    project.addComponent(controller(1.0, 1.0, 1.0).build());
    EXPECT_TRUE(evaluate(project).findings().empty());
}
