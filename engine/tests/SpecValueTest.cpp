// SYS-02: structured component specifications carry a data status.
#include <gtest/gtest.h>

#include <stdexcept>

#include "embedded_market/model/SpecValue.hpp"

using embedded_market::DataStatus;
using embedded_market::SpecValue;

TEST(SpecValueTest, DefaultIsMissingWithNoValue) {
    SpecValue<double> spec;
    EXPECT_EQ(spec.status(), DataStatus::Missing);
    EXPECT_FALSE(spec.hasValue());
    EXPECT_THROW(spec.value(), std::logic_error);
}

TEST(SpecValueTest, VerifiedCarriesValue) {
    auto spec = SpecValue<double>::verified(3.3);
    EXPECT_EQ(spec.status(), DataStatus::Verified);
    EXPECT_TRUE(spec.isVerified());
    EXPECT_DOUBLE_EQ(spec.value(), 3.3);
}

TEST(SpecValueTest, UnverifiedCarriesValueButIsNotVerified) {
    auto spec = SpecValue<int>::unverified(12);
    EXPECT_EQ(spec.status(), DataStatus::Unverified);
    EXPECT_FALSE(spec.isVerified());
    EXPECT_EQ(spec.value(), 12);
}

TEST(SpecValueTest, ContradictoryHasNoValue) {
    auto spec = SpecValue<double>::contradictory();
    EXPECT_EQ(spec.status(), DataStatus::Contradictory);
    EXPECT_FALSE(spec.hasValue());
    EXPECT_THROW(spec.value(), std::logic_error);
}
