// NFR-01 / SYS-03: the same project must always produce identical classifications and reasoning.
#include <gtest/gtest.h>

#include <string>

#include "TestParts.hpp"
#include "embedded_market/engine/CompatibilityEngine.hpp"

using namespace em_test;

namespace {

// A project that exercises Pass, Warning, and Conflict findings across every rule scope.
Project mixedProject() {
    Project project(supply(3.3));
    project.addComponent(mcu3v3().build());
    project.addComponent(sensor3v3("U1", {Interface::I2C}).build());
    project.addComponent(sensor3v3("U2", {Interface::SPI}).build());  // Interface conflict on I2C.
    PartBuilder unverified = sensor3v3("U3", {Interface::SPI});
    unverified.specs().extraPins = SpecValue<int>::unverified(1);
    project.addComponent(unverified.build());
    project.addComponent(PartBuilder("U4", ComponentRole::Peripheral)
                             .supply(4.5, 5.5)
                             .interfaces({Interface::SPI})
                             .logic(5.0, 3.5, 5.5)
                             .extraPins(0)
                             .build());
    project.addConnection(Connection("MCU1", "U1", Interface::I2C));
    project.addConnection(Connection("MCU1", "U2", Interface::I2C));
    project.addConnection(Connection("MCU1", "U3", Interface::SPI));
    project.addConnection(Connection("MCU1", "U4", Interface::SPI));
    return project;
}

void expectSameReport(const Report& expected, const Report& actual) {
    EXPECT_EQ(actual.status(), expected.status());
    ASSERT_EQ(actual.findings().size(), expected.findings().size());
    for (std::size_t i = 0; i < expected.findings().size(); ++i) {
        const Finding& a = actual.findings()[i];
        const Finding& e = expected.findings()[i];
        EXPECT_EQ(a.severity, e.severity) << "finding " << i;
        EXPECT_EQ(a.ruleId, e.ruleId) << "finding " << i;
        EXPECT_EQ(a.componentIds, e.componentIds) << "finding " << i;
        EXPECT_EQ(a.message, e.message) << "finding " << i;
    }
}

}  // namespace

TEST(DeterminismNfr01Test, MixedProjectCoversEverySeverity) {
    const Report report = CompatibilityEngine().evaluate(mixedProject());
    EXPECT_FALSE(report.withSeverity(Severity::Pass).empty());
    EXPECT_FALSE(report.withSeverity(Severity::Warning).empty());
    EXPECT_FALSE(report.withSeverity(Severity::Conflict).empty());
}

// NFR-01 acceptance: ten runs of the same configuration agree unanimously.
TEST(DeterminismNfr01Test, RepeatedRunsWithOneEngineAreIdentical) {
    const CompatibilityEngine engine;
    const Project project = mixedProject();
    const Report first = engine.evaluate(project);
    for (int run = 1; run < 10; ++run) {
        SCOPED_TRACE("run " + std::to_string(run));
        expectSameReport(first, engine.evaluate(project));
    }
}

TEST(DeterminismNfr01Test, FreshEnginesAndProjectsAreIdentical) {
    const Report first = CompatibilityEngine().evaluate(mixedProject());
    for (int run = 1; run < 10; ++run) {
        SCOPED_TRACE("run " + std::to_string(run));
        expectSameReport(first, CompatibilityEngine().evaluate(mixedProject()));
    }
}
