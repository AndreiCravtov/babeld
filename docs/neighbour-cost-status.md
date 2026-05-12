# Neighbour Cost Bias Status

## Current Stage

Stage 1 is implemented in babeld only.

Implemented:

- Added a local-control command parser for `neighbour-cost`.
- The command is accepted only after configuration has been finalised.
- The existing read-write local socket gate still applies.
- Valid command syntax returns `ok`.
- Invalid command syntax returns `bad`.
- The command is a no-op: it does not store state, change metrics, or emit neighbour updates.

Not implemented yet:

- Interface lookup.
- Link-local neighbour lookup.
- Per-neighbour bias state.
- Monitor/dump output fields for external bias.
- Metric recomputation.
- Expiry handling.
- Man page updates.

Verification:

- `make` passes with `-Wall`.

## Stage 1 Command Grammar

```text
neighbour-cost <ifname> <ipv6-address> <cost>
neighbour-cost <ifname> <ipv6-address> <cost> expires-ms <milliseconds>
```

Current parser validation:

- `<ifname>` must be a word.
- `<ipv6-address>` must parse as an IPv6 address.
- `<cost>` is interpreted as a non-negative bias and must be `0..65534`.
- `0` clears the intended bias in later stages.
- `expires-ms` requires a non-negative millisecond value.

Planned semantics:

```text
final_cost = babeld_existing_cost + external_bias + rtt_penalty
```

The external value is a bias added to babeld's native link cost, not a
replacement for it.
