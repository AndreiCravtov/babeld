# Neighbour Cost Implementation Plan

## Stage 1: Parser Surface

Status: done.

- Add `neighbour-cost` as a local-control command.
- Parse the intended cost-control grammar.
- Return `ok` for valid syntax.
- Return `bad` for malformed syntax.
- Do not store or apply any state.

## Stage 2: Semantic Validation Stub

Status: done.

Style priority: keep this feature visually and organisationally native to
babeld. Prefer the patterns already present in `configuration.c` and nearby
modules over introducing new architectural shapes. This includes local-control
response handling, parser out-parameter style, ownership conventions, and where
temporary or durable structs live.

- Add interface and non-creating neighbour lookup helpers.
- Add a command-handler validation/application function in `configuration.c`.
- Validate interface existence.
- Validate IPv6 link-local address.
- Validate neighbour existence on that interface.
- Require `bias-256 <int> coef-256 <nat> expiry-ms <nat>` in the
  command grammar.
- Treat `expiry-ms 0` as explicit no-expiry state.
- Return `no <reason>` for semantic failures.
- Keep metric behavior unchanged.
- Parser produces a heap-allocated request object, returned through a
  `struct neighbour_cost_request **`, matching the compound-object parser style
  used for filters, interface config, and keys. This is local-control parsing,
  so avoiding one short-lived allocation is not important.
- Validation/application is a separate command handler.
- Validation returns local-control response strings directly in `configuration.c`,
  matching nearby command handling such as `flush interface`.
- Interface and neighbour layers expose lookup helpers, not local-control response logic.

Architecture note: `configuration.c` is an acceptable temporary home for the
local-control parser and Stage 2 validation stub, because this command currently
enters through `parse_config_line`. Later stages should avoid making
`configuration.c` the long-term owner of neighbour cost behavior. Once real
state, notifications, metric updates, and expiry exist, keep the parser in the
local-control/config layer but move neighbour-domain mutation behind a neighbour
or cost-control API.

Struct-placement note: existing parsed compound objects are generally durable
objects declared outside `configuration.c`, such as `struct filter` in
`configuration.h` and `struct interface_conf` / `struct key` in `interface.h`.
The only structs currently defined in `configuration.c` besides
`neighbour_cost_request` are parser input-state helpers. Therefore
`neighbour_cost_request` should be treated as a temporary Stage 2 convenience,
not a pattern to expand. If it remains purely local and short-lived, consider
inlining it into the command parser/handler before shipping. If later stages
need a durable cost-control object, move that concept to the owning module
rather than growing private command DTOs in `configuration.c`.

Correction: the real mutator name should be reserved for a later stage and
should reflect the stable cost-control abstraction. Prefer a name such as
`set_neighbour_cost_control(...)`,
`set_neighbour_external_cost_params(...)`, or
`set_neighbour_cost_adjustment(...)`.

## Stage 3: Command And Monitor Schema Stabilization

Status: done.

- Stabilise the keyworded command grammar:
  `bias-256 <int> coef-256 <nat> expiry-ms <nat>`.
- Add neighbour monitor/dump fields for external cost-control state.
- Initially report
  `external-bias-256 0 external-coef-256 256 external-cost-expiry-ms 0`.
- Add accessors such as `neighbour_external_bias_256()`,
  `neighbour_external_coef_256()`, and
  `neighbour_external_cost_expiry_msecs()`; `local.c` should not reach into
  future state directly.
- Update downstream parsers against this stable line shape.

## Stage 4: Per-Neighbour State

- Add `external_bias_256`, `external_coef_256`, and
  `external_cost_expiry` to `struct neighbour`.
- Add helpers to read external cost-control state and remaining expiry time.
- Add a real cost-control mutator, e.g. `set_neighbour_cost_control(...)`.
- Make the mutator store and clear the fixed-point transform.
- Store positive `expiry-ms` values as monotonic `now + expiry_ms` deadlines;
  store `0` as no expiry.
- Preserve visible neutral state with positive expiry rather than collapsing it:
  `bias-256 0 coef-256 256 expiry-ms N` should report a neutral transform with
  a remaining expiry until it expires back to neutral/no-expiry.
- Emit neighbour notifications when visible cost-control state changes, even before
  metric integration exists.

## Stage 5: Metric Integration

- Keep existing liveness checks first.
- Compute the existing wired or ETX base cost.
- Apply the fixed-point external transform.
- Preserve RTT penalty handling.
- Call `update_neighbour_metric()` when the effective cost changes.

## Stage 6: Expiry

- Schedule neighbour checks for expiring external cost-control state.
- Clear expired positive-timeout external cost-control state in
  `check_neighbours()`.
- Emit neighbour change events through existing update paths.
- Include the next external-cost expiry in the returned check interval.

## Stage 7: Man Page

- Update `babeld.man`.
- Document command syntax, bias semantics, response behavior, and expiry.
- Make clear this is a local control socket command, not a Babel wire protocol extension.

## Stage 8: Manual Verification

- Add local socket transcript examples.
- Verify accepted commands, rejected commands, monitor events, route metric changes, and expiry fallback.

## Future Direction: Decimal Transform

- Consider accepting decimal `bias` and `coef` keywords in addition to the
  fixed-point `bias-256` and `coef-256` keywords.
- Keep backward compatibility for the current command grammar unless a clear
  versioned compatibility path exists.
- See `docs/neighbour-cost-future-directions.md`.
