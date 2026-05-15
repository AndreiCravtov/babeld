# Neighbour Cost Local Control Protocol

## Motivation

The goal is to let an external controller, such as a link profiler, apply local
per-neighbour cost control without extending the Babel protocol. The command
lives on babeld's local control socket. Peers only see the normal route metrics
that result from local route selection.

## Terminology

`neighbour-cost` is the stable cost-control command. "External cost control" is
the umbrella concept: it covers local external adjustment of babeld's native
per-neighbour link cost.

Stage 3 stabilizes the command and monitor schema for a fixed-point linear
transform:

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

`babeld_native_base_cost` means babeld's wired or ETX cost before RTT penalty.
Liveness checks remain outside the transform: an unusable neighbour still has
cost `INFINITY`. The Stage 5 implementation should use a wide signed integer
for `raw_256`.

## Command

```text
neighbour-cost <ifname> <link-local-neighbour> bias-256 <int> coef-256 <nat> expiry-ms <nat>
```

The keyword order is fixed. Whitespace and trailing comments follow the existing
configuration parser rules.

Examples:

```text
neighbour-cost en2 fe80::1234 bias-256 40960 coef-256 256 expiry-ms 30000
neighbour-cost en2 fe80::1234 bias-256 0 coef-256 128 expiry-ms 0
neighbour-cost en2 fe80::1234 bias-256 0 coef-256 256 expiry-ms 0
```

`bias-256` is the additive term, scaled by 256:

```text
bias-256 256    -> +1.0 cost
bias-256 -128   -> -0.5 cost
bias-256 40960  -> +160.0 cost
```

For Stage 3, the accepted range is:

```text
-(65534 * 256) .. +(65534 * 256)
```

`coef-256` is the multiplicative term, scaled by 256:

```text
coef-256 0    -> 0.0x native base cost
coef-256 128  -> 0.5x native base cost
coef-256 256  -> 1.0x native base cost
coef-256 512  -> 2.0x native base cost
```

For Stage 3, the accepted range is:

```text
0 .. 65535
```

That represents coefficients from `0.0` to roughly `255.996`.

`expiry-ms` is mandatory. It is a relative millisecond timeout parsed as a
natural integer. `expiry-ms 0` means no expiry is scheduled. `<int>` and
`<nat>` use babeld's existing `getint()` parser; this is not a strict
decimal-only grammar.

The neutral command is:

```text
neighbour-cost en2 fe80::1234 bias-256 0 coef-256 256 expiry-ms 0
```

## Timing Model

The command uses a relative millisecond timeout rather than a Unix timestamp.
This matches babeld's existing scheduling model: babeld updates the global
`now` from a monotonic `gettime()`, stores future deadlines as `struct timeval`
values derived from `now`, and reports elapsed or remaining time with
`timeval_minus_msec()`.

Unix timestamps would use wall-clock time, which is the wrong default for this
codebase. They would also require the external controller and babeld to agree on
wall-clock time. A relative timeout can be consumed directly as
`now + expiry_ms`; `0` can use the codebase's existing zero-time sentinel style
for "no scheduled timeout".

## Response Model

This command follows the existing local socket model:

- `ok`: command was accepted.
- `bad`: malformed command syntax.
- `no <reason>`: syntactically valid but not applicable.

Stage 3 produces `ok`, `bad`, and semantic `no ...` responses, but does not yet
store or apply any cost-control state.

Internally, syntax parsing returns values through local out-parameters. Semantic
validation and mutation happen in the `neighbour-cost` command branch in
`configuration.c`. For now, semantic rejections return local-control `no ...`
strings directly, matching nearby commands such as `flush interface`.

Current `no` cases:

- no such interface
- address is not link-local
- no such neighbour on that interface

## Monitor Output

Neighbour monitor and dump lines include explicit external cost-control state
while keeping `cost` as the final effective link cost:

```text
change neighbour ... rxcost 96 txcost 96 external-bias-256 0 external-coef-256 256 external-cost-expiry-ms 0 cost 96
```

New neighbours report the neutral transform and no expiry:

```text
external-bias-256 0 external-coef-256 256 external-cost-expiry-ms 0
```

`external-cost-expiry-ms 0` has the same no-expiry meaning as command input
`expiry-ms 0`.

Neutral state with a positive expiry is valid:

```text
neighbour-cost en2 fe80::1234 bias-256 0 coef-256 256 expiry-ms 30000
```

In Stage 4, that creates visible neutral state with a scheduled expiry. It has
no metric effect, but monitor output shows the remaining expiry until active
expiry clearing is implemented in Stage 6.

Stage 4 stores positive `expiry-ms` values as monotonic deadlines and reports
the remaining timeout. It does not yet clear expired state; before Stage 6, a
stored deadline that has already passed can report `external-cost-expiry-ms 0`.

## Stage 4 Transcript

Stage 4 validates the full command grammar and stores the transform:

```text
> neighbour-cost en2 fe80::1234 bias-256 40960 coef-256 256 expiry-ms 30000
ok
> dump
add neighbour ... external-bias-256 40960 external-coef-256 256 external-cost-expiry-ms 29998 cost ...
```

Metric computation is unchanged until Stage 5, so `cost` remains babeld's
native effective link cost.

## Future Decimal Syntax

A future local-control syntax may accept decimal terms, for example plain
`bias` and `coef` keywords. If that happens, the fixed-point fields should remain
available for compatibility or be replaced only through an explicit versioned
compatibility plan.
