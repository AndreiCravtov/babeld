# Zero-Interface Startup Lab Workflow

## Local Smoke Test

Build normally:

```text
nix develop -c make clean
nix develop -c make
```

Start babeld with no positional interfaces:

```text
./babeld -p 16697 \
  -G /tmp/babeld-zero-if.sock \
  -S /tmp/babeld-zero-if.state \
  -I /tmp/babeld-zero-if.pid \
  -C "skip-kernel-setup true" \
  -C "kernel-install false" \
  -C "random-id true"
```

Query the local control socket:

```text
printf '%s\n' 'dump' | nc -N -U /tmp/babeld-zero-if.sock
```

Expected result:

- the socket exists;
- the usual local-control prelude is emitted;
- `dump` completes with `ok`;
- no interface lines are present until one is added.

Add an interface after startup:

```text
printf '%s\n' 'interface <ifname>' | nc -N -U /tmp/babeld-zero-if.sock
```

Then dump again:

```text
printf '%s\n' 'dump' | nc -N -U /tmp/babeld-zero-if.sock
```

Expected result:

- a valid interface appears if `<ifname>` exists and can be configured;
- if `<ifname>` cannot become usable, babeld keeps the existing interface
  setup semantics and the interface may remain reported as down.

Stop the daemon:

```text
kill -INT "$(cat /tmp/babeld-zero-if.pid)"
```

## Notes

- Use a non-default Babel port during lab work to avoid colliding with any
  running babeld instance.
- `skip-kernel-setup true` avoids sysctl changes during local smoke tests.
- `nc -N` shuts down the socket after stdin reaches EOF; without it, some
  `nc` implementations keep the local-control connection open.
- This workflow intentionally does not require a managed interface at process
  start.
