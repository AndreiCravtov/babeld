# Neighbour Cost Implementation Plan

## Stage 1: Parser Surface

Status: done.

- Add `neighbour-cost` as a local-control command.
- Parse the intended bias grammar.
- Return `ok` for valid syntax.
- Return `bad` for malformed syntax.
- Do not store or apply any state.

## Stage 2: Semantic Validation Stub

Status: done.

- Add interface and non-creating neighbour lookup helpers.
- Add a command-handler validation/application function in `configuration.c`.
- Validate interface existence.
- Validate IPv6 link-local address.
- Validate neighbour existence on that interface.
- Return `no <reason>` for semantic failures.
- Keep metric behavior unchanged.
- Parser produces a request object; validation/application is a separate command handler.
- Validation uses typed internal results, then maps to local-control response strings.
- Interface and neighbour layers expose lookup helpers, not local-control response logic.

Correction: the real mutator name should be reserved for a later stage and
should reflect the stable cost-control abstraction. Prefer a name such as
`set_neighbour_cost_control(...)`,
`set_neighbour_external_cost_params(...)`, or
`set_neighbour_cost_adjustment(...)` rather than a bias-only setter named as if
it replaced the whole cost.

## Stage 3: Observable State Fields

- Add neighbour monitor/dump fields for external-bias state.
- Initially report `external-bias 0 external-bias-expiry-ms 0`.
- Add accessors such as `neighbour_external_bias()` and
  `neighbour_external_bias_expiry_msecs()`; `local.c` should not reach into
  future state directly.
- Update downstream parsers against this stable line shape.

## Stage 4: Per-Neighbour State

- Add `external_bias` and `external_bias_expiry` to `struct neighbour`.
- Add helpers to read bias state and remaining expiry time.
- Add a real cost-control mutator, e.g. `set_neighbour_cost_control(...)`.
- Make the mutator store and clear the bias.
- Emit neighbour notifications when visible bias state changes, even before
  metric integration exists.

## Stage 5: Metric Integration

- Keep existing liveness checks first.
- Compute the existing wired or ETX base cost.
- Add the external bias when non-zero.
- Preserve RTT penalty handling.
- Call `update_neighbour_metric()` when the effective cost changes.

## Stage 6: Expiry

- Schedule neighbour checks for expiring biases.
- Clear expired biases in `check_neighbours()`.
- Emit neighbour change events through existing update paths.
- Include the next external-bias expiry in the returned check interval.

## Stage 7: Man Page

- Update `babeld.man`.
- Document command syntax, bias semantics, response behavior, and expiry.
- Make clear this is a local control socket command, not a Babel wire protocol extension.

## Stage 8: Manual Verification

- Add local socket transcript examples.
- Verify accepted commands, rejected commands, monitor events, route metric changes, and expiry fallback.

## Future Direction: Linear Transform

- Consider generalising bias-only semantics to
  `C * babeld_existing_cost + external_bias`.
- Keep the MVP bias-only until the external scorer actually needs replacement or proportional scaling.
- Preserve backward compatibility for the current command grammar; future scale
  or coefficient parameters should be optional extensions.
- Keep monitor field `external-bias` as the additive `B` term. If a coefficient
  lands later, add a new field such as `external-cost-coefficient` or
  `external-cost-scale` instead of renaming `external-bias`.
- See `docs/neighbour-cost-future-directions.md`.
