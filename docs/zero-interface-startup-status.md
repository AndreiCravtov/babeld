# Zero-Interface Startup Status

## Current State

Implemented in babeld only.

Done:

- Zero positional interface arguments are accepted.
- `babel_main` no longer aborts when the managed-interface list is empty after
  configuration finalisation and positional argument processing.
- The existing local-control `interface <ifname>` command remains the path for
  adding interfaces after startup.
- Usage output and `babeld.man` document optional startup interfaces.
- Branch-local docs describe motivation, architecture, implementation plan, and
  lab workflow.

Not changed:

- No Babel protocol changes.
- No automatic interface discovery.
- No `babblerd` changes.
- No new command-line flag.

## Verification

Passed in this working tree:

- `nix develop -c make test`
- `nix develop -c make`
- `nix develop -c make babeld.html`
- `nix develop -c git diff --check`
- `nix develop -c mandoc -Tlint babeld.man` still reports only known
  pre-existing manpage diagnostics.
- no-interface local-control smoke test:
  - start babeld with no positional interfaces;
  - query `dump` through the read-write local socket;
  - verify the snapshot has no interface lines and ends with `ok`.
- post-start interface smoke test:
  - start babeld with no positional interfaces;
  - send `interface lo` through the read-write local socket;
  - verify a later `dump` reports `add interface lo ...`.
