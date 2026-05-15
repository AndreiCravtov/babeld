# Neighbour Cost Status

## Current Stage

Stage 6 is implemented in babeld only.

Implemented:

- Added a local-control command parser for `neighbour-cost`.
- The command is accepted only after configuration has been finalised.
- The existing read-write local socket gate still applies.
- Valid command syntax and semantics return `ok`.
- Invalid command syntax returns `bad`.
- Semantic validation failures return `no <reason>`.
- Accepted commands store per-neighbour external cost-control state.
- Implemented semantic validation for interface existence, link-local address,
  and existing neighbour lookup.
- Parsing returns values through out-parameters before validation/application,
  avoiding a one-off request struct and matching the local fixed-schema parser
  style.
- Semantic validation returns local-control response strings directly in
  `configuration.c`, matching nearby command handling such as `flush interface`.
- `interface.c` exposes interface lookup, and `neighbour.c` exposes
  non-creating neighbour lookup; no validation-only setter is exported.
- Neighbour monitor/dump output includes `external-bias-256` and
  `external-coef-256` fields.
- New neighbours initialise to the neutral transform:
  `bias-256 0`, `coef-256 256`.
- `neighbour_external_cost_configure()` stores bias and coefficient state on
  the neighbour.
- `neighbour_cost()` applies the fixed-point external transform to babeld's
  native wired or ETX base cost.
- RTT penalty is preserved outside the external coefficient.
- Liveness checks still short-circuit to `INFINITY` before the transform.
- Manual external cost-control changes call `update_neighbour_metric()`, which
  recalculates routes through the neighbour and emits the neighbour monitor
  update through the existing route path.
- Expiry is out of scope. External cost control is set-and-forget and must be
  changed or reset explicitly by another local-control command.
- `babeld.man` documents the local-control command syntax, fixed-point
  semantics, response behavior, monitor fields, and set-and-forget lifetime.

Verification:

- `nix develop -c make` passes with `-Wall`.
- `nix develop -c make test` passes after cleaning stale non-test objects.

## Command Grammar

```text
neighbour-cost <ifname> <ipv6-address> bias-256 <int> coef-256 <nat>
```

Current parser validation:

- `<ifname>` must be a word.
- `<ipv6-address>` must parse as an IPv6 address.
- `bias-256` is mandatory and must be followed by a signed integer in
  `-(65534 * 256)..+(65534 * 256)`.
- `coef-256` is mandatory and must be followed by a non-negative integer in
  `0..65535`.
- `<int>` and `<nat>` inherit the existing `getint()` base-0 syntax; they are
  not documented as strict decimal-only values.

Current semantic validation:

- `<ifname>` must name an existing babeld interface.
- `<ipv6-address>` must be link-local.
- The neighbour must already exist on the named interface.

Current semantic failure responses:

- `no No such interface`
- `no Address is not link-local`
- `no No such neighbour`

## Monitor/Dump Output

Neighbour local-control lines now include schema-stable external cost-control
fields:

```text
change neighbour ... rxcost 96 txcost 96 external-bias-256 0 external-coef-256 256 cost 96
```

The fields report the stored per-neighbour transform. With no expiry path, these
values remain until replaced, reset to neutral, or removed with the neighbour.

## Metric Semantics

```text
raw_256 = coef_256 * babeld_native_base_cost
        + bias_256
        + 256 * rtt_penalty

if raw_256 <= 256:
    final_cost = 1
else if raw_256 >= INFINITY * 256 - 128:
    final_cost = INFINITY
else:
    final_cost = (raw_256 + 128) / 256
```

Liveness checks remain outside the transform: an unusable neighbour still has
cost `INFINITY`. Metric integration computes `raw_256` in a wide signed integer
and clamps once at the final output boundary.

## Stage 5 Transcript

Stage 5 accepts the full schema, stores the transform on the neighbour, and
uses it for route metric calculation:

```text
> neighbour-cost en2 fe80::1234 bias-256 40960 coef-256 256
ok
> dump
add neighbour ... external-bias-256 40960 external-coef-256 256 cost ...
```
