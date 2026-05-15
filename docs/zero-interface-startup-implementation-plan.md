# Zero-Interface Startup Implementation Plan

## Stage 1: Remove The Fatal Startup Check

Status: done.

- Keep normal configuration finalisation.
- Keep positional interface handling for users that still pass interfaces on
  the command line.
- Make `interfaces == NULL` non-fatal after positional interfaces are processed.
- Leave the event loop, sockets, route checks, timers, and local-control setup
  unchanged.

## Stage 2: Documentation

Status: done.

- Update usage output to show optional positional interfaces.
- Update `babeld.man` to state that the interface list may be empty.
- Document the local-control `interface <ifname>` path as the way to add links
  after startup.
- Add branch docs under `docs/`.

## Stage 3: Verification

Status: done.

- Build with `nix develop -c make`.
- Run existing tests with `nix develop -c make test`.
- Start babeld with no positional interfaces, a read-write local socket, and
  `skip-kernel-setup true`.
- Confirm the local socket emits the usual startup prelude.
- Confirm `dump` returns an empty-but-valid snapshot and `ok`.
- Confirm `interface <ifname>` is accepted when a valid interface name is used.

## Future Babblerd Follow-Up

Out of scope for this branch.  The expected later `babblerd` change is:

- spawn babeld immediately without an initial interface argument;
- keep using the existing `interface <ifname>` local-control command for every
  discovered usable interface.
