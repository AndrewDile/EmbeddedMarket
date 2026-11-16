// FR-04 / OBJ4 / SYS-02 / SYS-04: uncertain data must trigger manual review, identify the
// component causing it, and never produce an all-checks-passed result. Runs the full default engine.
#include <gtest/gtest.h>

#include <limits>
#include <string>

#include "TestParts.hpp"
#include "embedded_market/engine/CompatibilityEngine.hpp"

using namespace em_test;

namespace {

// MCU1 -> U1 over I2C; fully compatible when every spec is verified.
Report evaluate(const PartBuilder& sensor) {
    Project project(supply(3.3));
    project.addComponent(mcu3v3().build());
    project.addComponent(sensor.build());
    project.addConnection(Connection("MCU1", "U1", Interface::I2C));
    return CompatibilityEngine().evaluate(project);
}

PartBuilder baselineSensor() { return sensor3v3("U1", {Interface::I2C}); }

// Acceptance: manual review (Warning), U1 identified, and no passing result overall.
void expectManualReviewFor(const Report& report, const std::string& text) {
    EXPECT_EQ(report.status(), Severity::Warning);
    bool found = false;
    for (const Finding& f : report.withSeverity(Severity::Warning)) {
        if (f.involves("U1") && f.message.find(text) != std::string::npos) found = true;
    }
    EXPECT_TRUE(found) << "no Warning for U1 mentioning '" << text << "'";
}

}  // namespace

TEST(UncertaintyFr04Test, BaselineProjectPasses) {
    EXPECT_EQ(evaluate(baselineSensor()).status(), Severity::Pass);
}

// FR-04 case 1: missing operating supply voltage.
TEST(UncertaintyFr04Test, MissingSupplyVoltageRequiresManualReview) {
    PartBuilder sensor = baselineSensor();
    sensor.specs().supplyVoltageMin = SpecValue<double>::missing();
    sensor.specs().supplyVoltageMax = SpecValue<double>::missing();
    expectManualReviewFor(evaluate(sensor), "supply voltage is missing");
}

// FR-04 case 2: missing interface list.
TEST(UncertaintyFr04Test, MissingInterfaceListRequiresManualReview) {
    PartBuilder sensor = baselineSensor();
    sensor.specs().interfaces = SpecValue<InterfaceSet>::missing();
    expectManualReviewFor(evaluate(sensor), "supported interfaces is missing");
}

// FR-04 case 3: contradictory supply range.
TEST(UncertaintyFr04Test, ContradictorySupplyRangeRequiresManualReview) {
    PartBuilder sensor = baselineSensor();
    sensor.specs().supplyVoltageMin = SpecValue<double>::contradictory();
    sensor.specs().supplyVoltageMax = SpecValue<double>::contradictory();
    expectManualReviewFor(evaluate(sensor), "supply voltage has contradictory values");
}

// FR-04 case 4: unverified pin count.
TEST(UncertaintyFr04Test, UnverifiedPinCountRequiresManualReview) {
    PartBuilder sensor = baselineSensor();
    sensor.specs().extraPins = SpecValue<int>::unverified(0);
    expectManualReviewFor(evaluate(sensor), "U1 extra pin count");
}

TEST(UncertaintyTest, ContradictoryLogicLevelRequiresManualReview) {
    PartBuilder sensor = baselineSensor();
    sensor.specs().logicOutputHigh = SpecValue<double>::contradictory();
    expectManualReviewFor(evaluate(sensor), "contradictory");
}

// Invalid values are treated as uncertain too.
TEST(UncertaintyTest, InvalidValueRequiresManualReview) {
    PartBuilder sensor = baselineSensor();
    sensor.specs().supplyVoltageMax = SpecValue<double>::verified(std::numeric_limits<double>::infinity());
    expectManualReviewFor(evaluate(sensor), "invalid value");
}

// A known conflict stays visible alongside uncertain data.
TEST(UncertaintyTest, ConflictOutranksUncertainty) {
    PartBuilder sensor = baselineSensor();
    sensor.interfaces({Interface::SPI});                         // Conflict on the I2C link.
    sensor.specs().extraPins = SpecValue<int>::unverified(0);    // Warning.
    const Report report = evaluate(sensor);
    EXPECT_EQ(report.status(), Severity::Conflict);
    EXPECT_FALSE(report.withSeverity(Severity::Warning).empty());
}
