# Neighbour Cost Lab Workflow

## Two-Node Manual Verification

Stage 7 was verified on two macOS hosts connected over Thunderbolt.  The
Thunderbolt interfaces are expected to remain stable for this lab pair:

```text
e4: en3
e2: en2
```

Use `/tmp` on the remote hosts as disposable workspace.  It may be erased on
restart, so the expected flow is to publish the branch from the local checkout
and recreate the remote checkout when needed.

From the local checkout:

```text
git status --short
git add <expected-files>
git commit -m "neighbour cost lab update"
git push origin custom-neighbour-cost
```

Then prepare each remote host.  Use the SSH URL when the host has GitHub SSH
access configured; otherwise use the HTTPS URL.

```text
ssh e4@e4
cd /tmp
git clone --recursive git@github.com:AndreiCravtov/babeld.git babeld-stage7
cd babeld-stage7
git checkout custom-neighbour-cost
git submodule update --init --recursive
nix develop -c make clean
nix develop -c make
```

```text
ssh e2@e2
cd /tmp
git clone --recursive git@github.com:AndreiCravtov/babeld.git babeld-stage7
cd babeld-stage7
git checkout custom-neighbour-cost
git submodule update --init --recursive
nix develop -c make clean
nix develop -c make
```

HTTPS clone fallback:

```text
git clone --recursive https://github.com/AndreiCravtov/babeld.git babeld-stage7
```

If `/tmp/babeld-stage7` already exists, either use a fresh directory name or
refresh the existing checkout:

```text
cd /tmp/babeld-stage7
git fetch origin
git checkout custom-neighbour-cost
git reset --hard origin/custom-neighbour-cost
git submodule update --init --recursive
```

Build normally inside the dev shell:

```text
nix develop -c make clean
nix develop -c make
```

Rediscover the current link-local addresses for each run; do not reuse an old
transcript's addresses.  On each host, inspect the expected Thunderbolt
interface and confirm the peer is reachable with link-local multicast:

```text
ifconfig en3        # on e4
ping6 -c 3 ff02::1%en3

ifconfig en2        # on e2
ping6 -c 3 ff02::1%en2
```

Record the peer's `fe80::...` address from `ifconfig`, `ping6`, or the
`neighbour` lines in `dump`.  The address passed to `neighbour-cost` should not
include a `%scope`; the interface argument supplies the scope:

```text
neighbour-cost <ifname> <peer-fe80-address-without-percent-scope> bias-256 0 coef-256 256
```

The daemons were run as root on a non-default Babel port with kernel route
installation disabled:

```text
./babeld -p 16696 -G /tmp/babeld-e4.sock \
  -S /tmp/babeld-stage7/state-e4 \
  -I /tmp/babeld-stage7/babeld-e4.pid \
  -L /tmp/babeld-stage7/babeld-e4.log \
  -C "skip-kernel-setup true" \
  -C "kernel-install false" \
  -C "random-id true" \
  en3

./babeld -p 16696 -G /tmp/babeld-e2.sock \
  -S /tmp/babeld-stage7/state-e2 \
  -I /tmp/babeld-stage7/babeld-e2.pid \
  -L /tmp/babeld-stage7/babeld-e2.log \
  -C "skip-kernel-setup true" \
  -C "kernel-install false" \
  -C "random-id true" \
  en2
```

## Local-Control Checks

Use `nc -U` to send one-shot commands to the local-control socket:

```text
printf '%s\n' 'dump' | sudo nc -U /tmp/babeld-e4.sock
printf '%s\n' 'neighbour-cost en3 <e2-fe80> bias-256 40960 coef-256 256' | sudo nc -U /tmp/babeld-e4.sock
```

For monitor output, keep an interactive socket client open, then type
`monitor` and apply updates from another terminal:

```text
sudo nc -U /tmp/babeld-e4.sock
```

If `nc -U` is unavailable, `socat` can be used instead:

```text
printf '%s\n' 'dump' | sudo socat - UNIX-CONNECT:/tmp/babeld-e4.sock
```

Semantic failures on `e4`:

```text
> neighbour-cost nosuch <e2-fe80> bias-256 0 coef-256 256
no No such interface

> neighbour-cost en3 2001:db8::1 bias-256 0 coef-256 256
no Address is not link-local

> neighbour-cost en3 fe80::1 bias-256 0 coef-256 256
no No such neighbour
```

Malformed syntax:

```text
> neighbour-cost en3 <e2-fe80> bias-256 0 coef-256
bad
```

Accepted cost update:

```text
> neighbour-cost en3 <e2-fe80> bias-256 40960 coef-256 256
ok
> dump
add neighbour ... external-bias-256 40960 external-coef-256 256 cost 256
add route ... metric 256 refmetric 0 via <e2-fe80> if en3
```

Neutral reset:

```text
> neighbour-cost en3 <e2-fe80> bias-256 0 coef-256 256
ok
> dump
add neighbour ... external-bias-256 0 external-coef-256 256 cost 96
add route ... metric 96 refmetric 0 via <e2-fe80> if en3
```

Monitor output during update and reset:

```text
change route ... metric 256 refmetric 0 via <e2-fe80> if en3
change neighbour ... external-bias-256 40960 external-coef-256 256 cost 256
change route ... metric 96 refmetric 0 via <e2-fe80> if en3
change neighbour ... external-bias-256 0 external-coef-256 256 cost 96
```

## Notes

- `kernel-install false` still reports routes as `installed yes` on the local
  control socket; it only skips the kernel route modification.
- The local UNIX sockets were owned by root, so the transcript commands were
  run through `sudo`.
- The route metric changed immediately after `neighbour-cost`, which verifies
  the `neighbour_external_cost_configure()` to `update_neighbour_metric()` path.
