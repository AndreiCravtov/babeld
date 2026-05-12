# Neighbour Cost Future Directions

## Cost Control Terminology

External cost control is the stable feature concept. The MVP exposes only the
additive bias term. A future version may expose a coefficient/scale term too.

The term `external-bias` should remain the additive `B` term even if a
coefficient is added later.

## Linear Cost Transform

The MVP uses a bias-only model:

```text
final_cost = babeld_existing_cost + external_bias + rtt_penalty
```

A future extension could generalise this to a linear transform:

```text
final_cost = C * babeld_existing_cost + external_bias + rtt_penalty
```

This can model both useful paradigms:

- `C = 1`: preserve babeld's native cost and add only `external_bias`.
- `C = 0`: replace babeld's native cost with the externally supplied bias term,
  effectively treating `external_bias` as a complete externally
  computed cost.
- `C > 1`: penalise links proportionally to babeld's native cost.
- `0 < C < 1`: make links more attractive while still preserving babeld's native
  ordering signal.

The implementation should still keep liveness checks outside the transform:

```text
if interface down or txcost/rxcost infinite:
    final_cost = INFINITY
else:
    final_cost = C * babeld_existing_cost + external_bias + rtt_penalty
```

## Fixed-Point Encoding

If this is implemented on the local control socket, `C` should likely be fixed
point rather than floating point. For example, `C256 = 256` can mean `C = 1.0`:

```text
transformed = (C256 * babeld_existing_cost + 128) / 256 + external_bias
```

The neutral transform would be:

```text
C256 = 256
external_bias = 0
```

The bias-only MVP is equivalent to keeping `C256 = 256` and exposing only
`external_bias`.

## Compatibility

The existing command grammar:

```text
neighbour-cost <ifname> <link-local-neighbour> <bias> [expires-ms <milliseconds>]
```

should remain valid if a coefficient is added. Future scale/coefficient controls
should be optional extensions, for example:

```text
neighbour-cost <ifname> <link-local-neighbour> <bias> [coefficient-256 <value>] [expires-ms <milliseconds>]
```

Monitor output should keep `external-bias` and add a new field such as
`external-cost-coefficient` or `external-cost-scale`.
