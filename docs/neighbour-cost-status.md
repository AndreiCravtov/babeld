# Neighbour Cost Status

## Current Stage

Stage 2 is implemented in babeld only.

Implemented:

- Added a local-control command parser for `neighbour-cost`.
- The command is accepted only after configuration has been finalised.
- The existing read-write local socket gate still applies.
- Valid command syntax and semantics return `ok`.
- Invalid command syntax returns `bad`.
- Semantic validation failures return `no <reason>`.
- The command is still a no-op after validation: it does not store state, change metrics, or emit neighbour updates.
- Implemented semantic validation for interface existence, link-local address, and existing neighbour lookup.
- Parsing now produces a heap-allocated local request object before validation/application, matching the compound-object parser style in `configuration.c`.
- Semantic validation returns local-control response strings directly in `configuration.c`, matching nearby command handling such as `flush interface`.
- `interface.c` exposes interface lookup, and `neighbour.c` exposes non-creating neighbour lookup; no validation-only setter is exported.

Not implemented yet:

- Per-neighbour bias state.
- A real cost-control mutator.
- Monitor/dump output fields for external bias.
- Metric recomputation.
- Expiry handling.
- Man page updates.

Verification:

- `nix develop -c make` passes with `-Wall`.
- `nix develop -c make test` passes after cleaning stale non-test objects.

## Command Grammar

```text
neighbour-cost <ifname> <ipv6-address> <bias>
neighbour-cost <ifname> <ipv6-address> <bias> expires-ms <milliseconds>
```

Current parser validation:

- `<ifname>` must be a word.
- `<ipv6-address>` must parse as an IPv6 address.
- `<bias>` must be `0..65534`.
- `0` clears the intended bias in later stages.
- `expires-ms` requires a non-negative millisecond value accepted by the
  existing `getint` parser; there is no separate post-parse upper-bound check.

Current semantic validation:

- `<ifname>` must name an existing babeld interface.
- `<ipv6-address>` must be link-local.
- The neighbour must already exist on the named interface.

Current semantic failure responses:

- `no No such interface`
- `no Address is not link-local`
- `no No such neighbour`

Planned semantics:

```text
final_cost = babeld_existing_cost + external_bias + rtt_penalty
```

The external value is a bias added to babeld's native link cost, not a
replacement for it.
