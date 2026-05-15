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
- Add command-handler validation/application logic in `configuration.c`.
- Validate interface existence.
- Validate IPv6 link-local address.
- Validate neighbour existence on that interface.
- Require `bias-256 <int> coef-256 <nat>` in the command grammar.
- Return `no <reason>` for semantic failures.
- Keep metric behavior unchanged.
- Parser returns the parsed values through out-parameters, matching the simpler
  fixed-schema parser helper style in `configuration.c`.
- Validation and application are handled in the `neighbour-cost` command branch.
- Validation returns local-control response strings directly in
  `configuration.c`, matching nearby command handling such as `flush interface`.
- Interface and neighbour layers expose lookup helpers, not local-control
  response logic.

Architecture note: `configuration.c` is an acceptable temporary home for the
local-control parser and Stage 2 validation stub, because this command currently
enters through `parse_config_line`. Later stages should avoid making
`configuration.c` the long-term owner of neighbour cost behavior. Keep the
parser in the local-control/config layer but move durable neighbour-domain
mutation behind a neighbour or cost-control API.

Struct-placement note: existing parsed compound objects are generally durable
objects declared outside `configuration.c`, such as `struct filter` in
`configuration.h` and `struct interface_conf` / `struct key` in `interface.h`.
The only structs currently defined in `configuration.c` are parser input-state
helpers. The temporary `neighbour_cost_request` parser struct used during Stage
2 was removed before shipping Stage 4. The command now keeps parsed values as
locals in `configuration.c`, which better matches nearby fixed-schema
directives. If later stages need a durable cost-control object, move that
concept to the owning module rather than growing private command DTOs in
`configuration.c`.

Correction: the real mutator name should be reserved for a later stage and
should reflect the stable external cost-control abstraction. Stage 4 uses
`neighbour_external_cost_configure(...)`.

## Stage 3: Command And Monitor Schema Stabilization

Status: done.

- Stabilise the keyworded command grammar:
  `bias-256 <int> coef-256 <nat>`.
- Add neighbour monitor/dump fields for external cost-control state.
- Initially report `external-bias-256 0 external-coef-256 256`.
- Add neighbour-layer accessors such as `neighbour_external_bias_256()` and
  `neighbour_external_coef_256()`; `local.c` should not reach into neighbour
  state directly.
- Update downstream parsers against this stable line shape.

## Stage 4: Per-Neighbour State

Status: done.

- Add `external_bias_256` and `external_coef_256` to `struct neighbour`.
- Add helpers to read external cost-control state.
- Add a real cost-control mutator,
  `neighbour_external_cost_configure(...)`.
- Make the mutator store the fixed-point transform.
- Emit neighbour notifications when visible cost-control state changes, even
  before metric integration exists.
- Keep expiry out of scope: there is no `expiry-ms` grammar, no per-neighbour
  deadline state, no monitor expiry field, and no external-cost scheduling path
  in neighbour maintenance.
- Use a simple set-and-forget model: to change the
  transform, send another `neighbour-cost` command; to reset it, send the
  neutral `bias-256 0 coef-256 256` transform.
- Keep the historical `check_neighbours()` / `babeld.c` scheduling contract
  untouched by this feature.

## Stage 5: Metric Integration

Status: done.

- Keep existing liveness checks first.
- Compute the existing wired or ETX base cost.
- Apply the fixed-point external transform.
- Preserve RTT penalty handling.
- Call `update_neighbour_metric()` when manual external cost-control state
  changes, so routes through the neighbour are recalculated immediately.
- Use the external cost accessors, not raw stored bias/coef fields.
- Add targeted tests for neutral behavior, bias/coefficient behavior, clamp
  thresholds, RTT penalty preservation, and liveness short-circuiting.

## Stage 6: Man Page

Status: done.

- Update `babeld.man`.
- Document command syntax, bias/coefficient semantics, and response behavior.
- Make clear this is a local control socket command, not a Babel wire protocol
  extension.
- Document that there is no expiry; controllers must reset stale values
  explicitly.

## Stage 7: Manual Verification

Status: done.

- Add local socket transcript examples.
- Verify accepted commands, rejected commands, monitor events, route metric
  changes, and explicit neutral reset.
- Record the two-node workflow in `docs/neighbour-cost-lab-workflow.md`.

## Future Direction: Decimal Transform

- Consider accepting decimal `bias` and `coef` keywords in addition to the
  fixed-point `bias-256` and `coef-256` keywords.
- Keep backward compatibility for the current command grammar unless a clear
  versioned compatibility path exists.
- See `docs/neighbour-cost-future-directions.md`.
