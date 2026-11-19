# Compatibility Rules Engine

C++17 static library that checks an embedded project's components and connections against a
set of compatibility rules and explains every result.

## Prerequisites

- A C++17 compiler (g++, clang++, or MSVC), CMake 3.18+, and optionally Ninja
- GoogleTest, for the unit tests. If CMake can't find an installed copy, it downloads
  GoogleTest v1.15.2 into the build directory on the first configure (needs internet once) and
  builds it with your compiler. To use an installed copy instead:
  - MSYS2 MinGW64: `pacman -S mingw-w64-x86_64-gtest`
  - Ubuntu / WSL: `sudo apt install libgtest-dev`
  - macOS: `brew install googletest`

## Build and test

From the repository root:

```sh
cmake -S engine -B engine/build -G Ninja
cmake --build engine/build
ctest --test-dir engine/build --output-on-failure
```

JUnit-style XML results are written to `engine/build/test-results/`. Pass
`-DEM_BUILD_TESTS=OFF` to build only the library.

## Run in Docker

From the repository root:

```sh
docker build --target test engine                   # Build and run the unit tests on Linux
docker build --target service -t em-engine engine   # Build the HTTP service
docker run --rm -p 8081:8081 em-engine
```

The service is stateless and has two endpoints:

| Endpoint | Purpose |
|---|---|
| `GET /health` | Returns `{"status":"ok"}`. |
| `POST /evaluate` | Takes a whole project and returns its report. A malformed request returns HTTP 400 with `{"error": "..."}`. |

Request body (`constraints` and any individual spec may be omitted; an omitted spec is Missing):

```json
{
  "supply": {"min": 3.3, "max": 3.3},
  "constraints": {
    "powerBudgetMa": 200,
    "allowedInterfaces": ["I2C", "SPI", "GPIO"],
    "minClockMhz": 48,
    "minFlashKb": 256,
    "minRamKb": 64
  },
  "components": [
    {
      "id": "U1",
      "role": "controller",
      "specs": {"currentMa": {"value": 40, "status": "verified"}}
    }
  ],
  "connections": [{"upstream": "U1", "downstream": "U3", "interface": "SPI"}]
}
```

Spec fields: `supplyVoltageMin`, `supplyVoltageMax`, `interfaces`, `logicOutputHigh`,
`logicInputHighMin`, `logicInputMax`, `availableGpio`, `extraPins`, `currentMa`, `clockMhz`,
`flashKb`, `ramKb`. Each status is `verified`, `unverified`, `missing`, or `contradictory`.

Response body:

```json
{
  "status": "conflict",
  "checks": {"total": 33, "passed": 30, "warnings": 2, "conflicts": 1},
  "findings": [
    {
      "severity": "conflict",
      "ruleId": "logic-level",
      "componentIds": ["U3", "U1"],
      "message": "U1 -> U3 (SPI): U3 drives logic high at 5 V, above U1's maximum input voltage of 3.3 V (risk of damage)",
      "metrics": {"outputHighV": 5, "inputMaxV": 3.3}
    }
  ]
}
```

`status` and `severity` are `pass`, `warning`, or `conflict`. `checks` counts the findings in the
report, so `total` can change between runs as parts and connections change. `metrics` holds the numbers behind
some findings: `pinsUsed` and `pinsAvailable` (`pin-budget`), and `knownMa`, `budgetMa`, and
`unknownParts` (the `power-budget` project total).

## Concepts

| Type | Purpose |
|---|---|
| `SpecValue<T>` | A spec value plus its `DataStatus`: `Verified`, `Unverified`, `Missing`, or `Contradictory`. |
| `Component` / `ComponentSpecs` | A part and its normalized specs (operating supply range, interfaces, logic levels, available GPIO, extra pins). |
| `Connection` | An `upstream -> downstream` link over one `Interface` (I2C, SPI, UART, or GPIO). Either end can be any component, so chains such as MCU -> I2C mux -> sensor or MCU -> motor driver -> motor are just more connections. |
| `Project` | The user-declared supply range (`VoltageRange`) plus the component/connection graph. |
| `Rule` | Base class. Concrete rules derive from `ComponentRule`, `ConnectionRule`, or `ProjectRule`. |
| `RuleRegistry` | Owns the rules to run; `RuleRegistry::withDefaults()` has every built-in rule. |
| `CompatibilityEngine` | Runs every rule over a project and returns a `Report`. |
| `Report` / `Finding` | Each finding has a `Severity`, rule id, the components involved, and an explanation. The overall status is the most severe finding: `Conflict` > `Warning` > `Pass`. |

### Uncertainty policy

Rules read their inputs through `SpecGate`, which enforces the same policy everywhere:

- Missing, contradictory, or invalid input: the check is skipped and a **Warning** names the
  component and field.
- Unverified input: the check runs, but a pass becomes a **Warning**. A conflict stays a
  **Conflict**, and the message notes that the data was unverified.

Unverified data therefore never produces a pass.

## Built-in rules

| Id | Scope | Requirement | Checks |
|---|---|---|---|
| `spec-sanity` | Component | SYS-02 | Invalid values (non-finite, negative, inverted ranges) and unlikely values (outside `SanityLimits`) produce a Warning. |
| `supply-voltage` | Component | FR-02 | The **entire** declared supply range lies within the part's operating range (inclusive, with a 1e-6 V tolerance). Overlap alone is a Conflict. |
| `interface` | Connection | FR-03 | Both ends support the chosen interface. |
| `logic-level` | Connection | SYS-03 | In each direction, the driver's V_OH must be at most the receiver's maximum input voltage and at least its V_IH(min). |
| `pin-budget` | Project | FR-06 | Each host's connections fit within its available GPIO. I2C bus pins (2) and SPI bus pins (3) count once per host; SPI adds one chip-select per device; UART needs 2 per device; each part adds its extra pins. |
| `power-budget` | Project | SYS-03, FR-04 | The summed maximum current fits the project's power budget. An unknown current warns for that part and leaves the project total incomplete. Skipped if no budget is declared. |
| `project-constraints` | Project | SYS-03 | Controllers meet the declared minimum clock, flash, and RAM, and connections use allowed interfaces. Constraints that are not declared are skipped. |

Requirement-linked tests are named after their requirement (`SupplyVoltageFr02Test`,
`InterfaceRuleObj3Test`, `UncertaintyFr04Test`, `DeterminismNfr01Test`).

Known gaps: pin types and specific pin reservations (FR-06), I2C open-drain pull-up levels, a
separate supply per component, and I2C address collisions.

## Adding a rule

1. Derive from the scoped base that matches what the rule looks at, and implement `id()`,
   `description()`, and `evaluate(...)`. Use `SpecGate` to read specs, so the uncertainty
   policy is applied for you.
2. Register it in `RuleRegistry::withDefaults()` (or on a custom registry), and add the
   source file to `CMakeLists.txt`.
3. Add a test file under `tests/` and list it in `tests/CMakeLists.txt`.

To add a new scope (for example a shared bus, or a path through a multiplexer), add a
`RuleScope` value and an abstract base in `rules/Rule.hpp`, a list and `add` overload in
`RuleRegistry`, and a loop in `CompatibilityEngine::evaluate`.
