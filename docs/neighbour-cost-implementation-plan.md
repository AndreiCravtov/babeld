# Neighbour Cost Implementation Plan

## Stage 1: Parser Surface

Status: done.

- Add `neighbour-cost` as a local-control command.
- Parse the intended bias grammar.
- Return `ok` for valid syntax.
- Return `bad` for malformed syntax.
- Do not store or apply any state.

## Stage 2: Semantic Validation Stub

- Add a neighbour-layer setter function.
- Validate interface existence.
- Validate IPv6 link-local address.
- Validate neighbour existence on that interface.
- Return `no <reason>` for semantic failures.
- Keep metric behavior unchanged.

## Stage 3: Observable State Fields

- Add neighbour monitor/dump fields for external-bias state.
- Initially report `external-bias 0 external-bias-expiry-ms 0`.
- Update downstream parsers against this stable line shape.

## Stage 4: Per-Neighbour State

- Add `external_bias` and `external_bias_expiry` to `struct neighbour`.
- Add helpers to read bias state and remaining expiry time.
- Make the setter store and clear the bias.

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

- Consider generalising bias-only semantics to `A * babeld_existing_cost + B`.
- Keep the MVP bias-only until the external scorer actually needs replacement or proportional scaling.
- See `docs/neighbour-cost-future-directions.md`.
