# Neighbour Cost Local Control Protocol

## Motivation

The goal is to let an external controller, such as a link profiler, add a local
per-neighbour link cost bias without extending the Babel protocol. The command
lives on babeld's local control socket. The resulting route metrics will later
flow through normal Babel Updates.

## Command

```text
neighbour-cost <ifname> <link-local-neighbour> <bias> [expires-ms <milliseconds>]
```

The command name remains `neighbour-cost`, but the numeric argument is a
non-negative bias added to babeld's native cost.

Examples:

```text
neighbour-cost en2 fe80::1234 160 expires-ms 30000
neighbour-cost en2 fe80::1234 0
```

Bias `0` means clear the external bias. Non-zero values are added to babeld's
native per-neighbour link cost.

`expires-ms` is syntactically valid with any bias value, including `0`. An
expiry of `0` is accepted and means immediate expiry in later stages.

The planned final formula is:

```text
final_cost = babeld_existing_cost + external_bias + rtt_penalty
```

The external bias does not replace `babeld_existing_cost`; it only makes the
link less preferred by adding a non-negative penalty.

## Response Model

This command follows the existing local socket model:

- `ok`: command was accepted.
- `bad`: malformed command syntax.
- `no <reason>`: syntactically valid but not applicable.

Stage 1 only produces `ok` or `bad`, because semantic lookup is not wired yet.

Expected later `no` cases:

- no such interface
- address is not link-local
- no such neighbour on that interface
- bias expiry is invalid for the current neighbour state

## Monitor Output Target

Later stages should extend neighbour monitor lines with explicit external-bias
state while keeping `cost` as the final effective link cost:

```text
change neighbour ... rxcost 96 txcost 96 external-bias 160 external-bias-expiry-ms 24500 cost 256
change neighbour ... rxcost 96 txcost 96 external-bias 0 external-bias-expiry-ms 0 cost 96
```
