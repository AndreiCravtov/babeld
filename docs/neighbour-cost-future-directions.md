# Neighbour Cost Future Directions

## Linear Cost Transform

The MVP uses a bias-only model:

```text
final_cost = babeld_existing_cost + external_bias + rtt_penalty
```

A future extension could generalise this to a linear transform:

```text
final_cost = A * babeld_existing_cost + B + rtt_penalty
```

This can model both useful paradigms:

- `A = 1, B = external_bias`: preserve babeld's native cost and add a bias.
- `A = 0, B = external_cost`: replace babeld's native cost with an externally
  computed cost.
- `A > 1`: penalise links proportionally to babeld's native cost.
- `0 < A < 1`: make links more attractive while still preserving babeld's native
  ordering signal.

The implementation should still keep liveness checks outside the transform:

```text
if interface down or txcost/rxcost infinite:
    final_cost = INFINITY
else:
    final_cost = A * babeld_existing_cost + B + rtt_penalty
```

## Fixed-Point Encoding

If this is implemented on the local control socket, `A` should likely be fixed
point rather than floating point. For example, `A256 = 256` can mean `A = 1.0`:

```text
transformed = (A256 * babeld_existing_cost + 128) / 256 + B
```

The neutral transform would be:

```text
A256 = 256
B = 0
```

The bias-only MVP is equivalent to keeping `A256 = 256` and exposing only `B`.

