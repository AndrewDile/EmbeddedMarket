#pragma once

#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace embedded_market {

// How far the engine can trust a specification value pulled from a datasheet or other source.
enum class DataStatus {
    Verified,       // Value confirmed (e.g. reviewed by a person or matched across sources).
    Unverified,     // Value present but not yet confirmed.
    Missing,        // No value could be found.
    Contradictory,  // Sources disagree, so no single value can be trusted.
};

std::string toString(DataStatus status);

// A single specification value together with its DataStatus.
// Only Verified and Unverified values carry a value; Missing and Contradictory never do.
template <typename T>
class SpecValue {
public:
    SpecValue() = default;  // Missing

    static SpecValue verified(T value) { return SpecValue(std::move(value), DataStatus::Verified); }
    static SpecValue unverified(T value) { return SpecValue(std::move(value), DataStatus::Unverified); }
    static SpecValue missing() { return SpecValue(); }
    static SpecValue contradictory() {
        SpecValue spec;
        spec.status_ = DataStatus::Contradictory;
        return spec;
    }

    DataStatus status() const { return status_; }
    bool hasValue() const { return value_.has_value(); }
    bool isVerified() const { return status_ == DataStatus::Verified; }

    const T& value() const {
        if (!value_) {
            throw std::logic_error("SpecValue has no value (status: " + toString(status_) + ")");
        }
        return *value_;
    }

private:
    SpecValue(T value, DataStatus status) : value_(std::move(value)), status_(status) {}

    std::optional<T> value_;
    DataStatus status_ = DataStatus::Missing;
};

}  // namespace embedded_market
