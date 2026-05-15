# Neighbour Cost Future Directions

## Decimal Input

Stage 3 uses explicit fixed-point integer keywords:

```text
bias-256 <int> coef-256 <nat>
```

A future version may accept decimal keywords such as:

```text
bias <decimal> coef <decimal>
```

If decimal syntax is added, it should either coexist with the fixed-point
keywords or be introduced through a versioned compatibility path. Existing local
socket consumers may parse command grammar and monitor output strictly.

## Current Fixed-Point Transform

The fixed-point transform is:

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

`babeld_native_base_cost` means the wired or ETX link cost before the RTT
penalty is added. RTT penalty remains outside the external coefficient unless a
future design explicitly changes that.

Useful points:

- `coef-256 256`: preserve babeld's native base cost.
- `coef-256 0`: ignore babeld's native base cost.
- `coef-256 128`: halve babeld's native base cost.
- `coef-256 512`: double babeld's native base cost.
- `bias-256 256`: add one cost unit.
- `bias-256 -256`: subtract one cost unit.

The implementation should still keep liveness checks outside the transform:

```text
if interface down or txcost/rxcost infinite:
    final_cost = INFINITY
else:
    apply the final clamp/round algorithm
```

## Ranges

Stage 3 parser ranges:

```text
bias-256: -(65534 * 256) .. +(65534 * 256)
coef-256: 0 .. 65535
expiry-ms: 0 .. parser maximum
```

`bias-256` is larger than `coef-256` because it is measured in cost units, not
as a multiplier. `coef-256` is non-negative because a negative coefficient
inverts babeld's native cost signal: worse native links would receive a larger
reward. `coef-256 65535` is already roughly `255.996x`, which is far beyond
practical use and will often saturate after clamping.

Metric integration should use a wide signed integer for intermediate arithmetic
and clamp only at the final output boundary.

## Expiry Representation

Keep the local-control input as a relative millisecond timeout:

```text
expiry-ms <milliseconds>
```

Do not switch to Unix timestamps. babeld's internal `gettime()` is monotonic
when possible, and most internal scheduling stores deadlines as `struct timeval`
values computed from the current monotonic `now`. A Unix timestamp would be a
wall-clock value, would be vulnerable to clock changes or clock skew between the
controller and babeld, and would still need conversion into babeld's monotonic
deadline model.

Stage 4 should convert positive `expiry-ms` values into `now + expiry_ms`.
`expiry-ms 0` means no expiry and can be represented by a zero deadline.
