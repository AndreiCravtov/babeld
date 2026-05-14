# Neighbour Cost Footguns

## Local Control Is Not Babel Wire Protocol

`neighbour-cost` must stay on the local control socket. It should not create a
new Babel TLV or change RFC 8966 packet handling. Babel peers will only see the
normal route metrics that result from local route selection.

## Do Not Bypass Link Liveness

External bias must not make a dead neighbour usable. The existing checks for
interface state, IHU `txcost`, and Hello-derived `rxcost` must remain before the
bias is applied.

## Do Not Replace Babel's Native Cost

For the MVP, the external value is a non-negative bias:

```text
final_cost = babeld_existing_cost + external_bias + rtt_penalty
```

This preserves babeld's native wired/ETX signal and only makes selected links
less preferred.

## Avoid Metric Churn

An external profiler can produce noisy measurements. Later stages should expect
the controller to use smoothing or hysteresis. Babel remains loop-safe with
valid positive costs, but frequent cost changes can still create route churn.

## Avoid Overly Strict Semantics

Do not reject harmless input just because it is not expected to be useful. The
local control surface should enforce invariants that matter for correctness and
clear parsing, but avoid policy restrictions that make clients more brittle.

For example, `neighbour-cost IFADDR 0 expires-ms N` is a no-op that expires into
the same no-op. That is harmless, so it should be accepted rather than rejected.
Likewise, `expires-ms 0` is just immediate expiry, which is harmless.

## Expiry Is Required For Safety

Without expiry, a dead or wedged profiler could leave stale bias data in place.
The bias should be refresh-based in production use: the controller keeps
renewing it, and babeld falls back to unbiased scoring when renewal stops.

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
`external-bias` for the MVP additive term. If future linear cost control lands,
add separate coefficient/scale naming rather than renaming `external-bias`.

## Match Local Organisation

This change should feel native to babeld, not like a new subsystem grafted into
the parser. Before adding helper structs, callbacks, result types, or ownership
patterns, check whether `configuration.c` already uses that shape for similar
commands. Existing parsed compound objects are durable objects declared in
headers; purely local command request structs should be avoided unless they keep
an incremental stage materially clearer.
