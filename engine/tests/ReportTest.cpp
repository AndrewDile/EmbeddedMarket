// SYS-04: the report's overall status and finding lookups.
#include <gtest/gtest.h>

#include "embedded_market/engine/Report.hpp"

using namespace embedded_market;

TEST(ReportTest, EmptyReportPasses) {
    Report report;
    EXPECT_EQ(report.status(), Severity::Pass);
    EXPECT_TRUE(report.findings().empty());
}

TEST(ReportTest, StatusIsMostSevereFinding) {
    Report report;
    report.add({Severity::Pass, "r1", {"A"}, "ok"});
    EXPECT_EQ(report.status(), Severity::Pass);
    report.add({Severity::Warning, "r2", {"B"}, "unsure"});
    EXPECT_EQ(report.status(), Severity::Warning);
    report.add({Severity::Conflict, "r3", {"C"}, "bad"});
    EXPECT_EQ(report.status(), Severity::Conflict);
    report.add({Severity::Pass, "r4", {"D"}, "ok"});
    EXPECT_EQ(report.status(), Severity::Conflict);
}

TEST(ReportTest, ConflictKeepsWarningsVisible) {
    Report report;
    report.add({Severity::Conflict, "r1", {"A"}, "bad"});
    report.add({Severity::Warning, "r2", {"B"}, "unsure"});
    EXPECT_EQ(report.status(), Severity::Conflict);
    EXPECT_EQ(report.withSeverity(Severity::Warning).size(), 1u);
}

TEST(ReportTest, FiltersByComponentAndRule) {
    Report report;
    report.add({Severity::Pass, "interface", {"MCU1", "U1"}, "ok"});
    report.add({Severity::Conflict, "supply-voltage", {"U1"}, "bad"});
    report.add({Severity::Pass, "supply-voltage", {"MCU1"}, "ok"});

    EXPECT_EQ(report.forComponent("U1").size(), 2u);
    EXPECT_EQ(report.forComponent("MCU1").size(), 2u);
    EXPECT_EQ(report.fromRule("supply-voltage").size(), 2u);
    EXPECT_EQ(report.fromRule("interface").size(), 1u);
}
