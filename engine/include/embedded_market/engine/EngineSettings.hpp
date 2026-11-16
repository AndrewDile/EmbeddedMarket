#pragma once

namespace embedded_market {

// Bounds outside which a value is treated as unlikely and flagged for manual review.
struct SanityLimits {
    double maxPlausibleVoltage = 60.0;
    int maxPlausibleGpio = 500;
    int maxPlausibleExtraPins = 100;
};

struct EngineSettings {
    // Absorbs floating-point error in voltage comparisons. This is not an engineering margin.
    double voltageTolerance = 1e-6;
    SanityLimits limits;
};

}  // namespace embedded_market
