# Neighbour Cost Footguns

## Local Control Is Not Babel Wire Protocol

`neighbour-cost` must stay on the local control socket. It should not create a
new Babel TLV or change RFC 8966 packet handling. Babel peers will only see the
normal route metrics that result from local route selection.

## Do Not Bypass Link Liveness

External cost control must not make a dead neighbour usable. The existing checks
for interface state, IHU `txcost`, and Hello-derived `rxcost` must remain before
the transform is applied.

## Clamp Only At The Boundary

The fixed-point transform may produce a value outside Babel's finite metric
range, especially with signed bias values and large coefficients:

```text
raw_256 = coef_256 * babeld_native_base_cost
        + bias_256
        + 256 * rtt_penalty
```

Do the arithmetic in a wide signed type, then clamp the rounded final result
once at the protocol boundary. Do not clamp each term independently unless a
specific overflow or safety invariant requires it.

## Do Not Invert Native Cost By Default

`coef-256` is intentionally non-negative. A negative coefficient would reward
higher babeld native costs, causing worse links to become more attractive after
the final clamp. Use signed `bias-256` for preference shifts, and use
`coef-256 0` plus `bias-256` for replacement-style external scoring.

## Avoid Metric Churn

An external profiler can produce noisy measurements. Later stages should expect
the controller to use smoothing or hysteresis. Babel remains loop-safe with
valid positive costs, but frequent cost changes can still create route churn.

## Avoid Overly Strict Semantics

Do not reject harmless input just because it is not expected to be useful. The
local control surface should enforce invariants that matter for correctness and
clear parsing, but avoid policy restrictions that make clients more brittle.

For example, the neutral command
`neighbour-cost IFNAME ADDR bias-256 0 coef-256 256 expiry-ms N` is a no-op
that expires into the same no-op. That is harmless, so it should be accepted
rather than rejected. Likewise, `expiry-ms 0` is just "never expire", which is
harmless.

## Prefer Finite Expiry For Safety

Without finite expiry, a dead or wedged profiler could leave stale cost-control
data in place. The command still accepts `expiry-ms 0` for explicit no-expiry
state, but production controllers should normally use refresh-based finite
timeouts: the controller keeps renewing them, and babeld falls back to neutral
scoring when renewal stops.

## Local Socket Compatibility

Downstream consumers may parse local socket output strictly. When neighbour
monitor lines change, update consumers and tests in lockstep or add a clearly
versioned compatibility path.

## Keep Boundaries Honest

Do not name validation-only helpers as setters. Cost-control mutators should be
introduced only when they actually mutate neighbour state. Keep local-control
response strings in the local/config command layer; lower layers should return
typed results or domain objects.

Use `cost` for the long-lived umbrella feature and API concept. Use
`external-bias-256` for the additive fixed-point term and `external-coef-256`
for the multiplicative fixed-point term.

## Match Local Organisation

This change should feel native to babeld, not like a new subsystem grafted into
the parser. Before adding helper structs, callbacks, result types, or ownership
patterns, check whether `configuration.c` already uses that shape for similar
commands. Existing parsed compound objects are durable objects declared in
headers; purely local command request structs should be avoided unless they keep
an incremental stage materially clearer.
