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

The command and monitor schema use a fixed-point linear transform:

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
cost `INFINITY`. The metric implementation should use a wide signed integer for
`raw_256`.

## Command

```text
neighbour-cost <ifname> <link-local-neighbour> bias-256 <int> coef-256 <nat>
```

The keyword order is fixed. Whitespace and trailing comments follow the existing
configuration parser rules.

Examples:

```text
neighbour-cost en2 fe80::1234 bias-256 40960 coef-256 256
neighbour-cost en2 fe80::1234 bias-256 0 coef-256 128
neighbour-cost en2 fe80::1234 bias-256 0 coef-256 256
```

`bias-256` is the additive term, scaled by 256:

```text
bias-256 256    -> +1.0 cost
bias-256 -128   -> -0.5 cost
bias-256 40960  -> +160.0 cost
```

The accepted range is:

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

The accepted range is:

```text
0 .. 65535
```

That represents coefficients from `0.0` to roughly `255.996`.

`<int>` and `<nat>` use base-0 integer syntax, so decimal, octal-style, and
hexadecimal tokens accepted by `strtol(..., base 0)` are valid.  The
`neighbour-cost` parser checks overflow and the documented range before
narrowing the value to an `int`; oversized tokens are rejected as `bad`.

The neutral reset command is:

```text
neighbour-cost en2 fe80::1234 bias-256 0 coef-256 256
```

## Lifetime Model

External cost control is set-and-forget. Accepted state remains attached to the
neighbour until one of these happens:

- another `neighbour-cost` command replaces it;
- a neutral `bias-256 0 coef-256 256` command resets it;
- the neighbour itself is flushed.

There is intentionally no expiry timer in babeld. Controllers that require
freshness must implement refresh or reset policy outside babeld by sending a new
command.

## Response Model

This command follows the existing local socket model:

- `ok`: command was accepted.
- `bad`: malformed command syntax.
- `no <reason>`: syntactically valid but not applicable.

The current implementation produces `ok`, `bad`, and semantic `no ...`
responses. Accepted commands store per-neighbour external cost-control state and
immediately refresh route metrics that use that neighbour.

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
change neighbour ... rxcost 96 txcost 96 external-bias-256 0 external-coef-256 256 cost 96
```

New neighbours report the neutral transform:

```text
external-bias-256 0 external-coef-256 256
```

Manual `neighbour-cost` changes notify monitors immediately. The existing
neighbour maintenance path can also emit neighbour monitor updates when no
external-cost state changed, so monitor clients should treat neighbour lines as
state snapshots rather than precise edge-triggered events.

## Example Transcript

The command validates the full grammar, stores the transform, and uses it
for route metric calculation:

```text
> neighbour-cost en2 fe80::1234 bias-256 40960 coef-256 256
ok
> dump
add neighbour ... external-bias-256 40960 external-coef-256 256 cost ...
```

## Future Decimal Syntax

A future local-control syntax may accept decimal terms, for example plain
`bias` and `coef` keywords. If that happens, the fixed-point fields should remain
available for compatibility or be replaced only through an explicit versioned
compatibility plan.
