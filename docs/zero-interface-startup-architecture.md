# Zero-Interface Startup Architecture

## Motivation

`babblerd` wants to run babeld as a private control-plane process before any
physical link is ready for Babel.  The current `babblerd` workaround is to wait
for the first usable interface, then pass that interface as a positional babeld
argument at spawn time.  That makes the process lifecycle depend on interface
discovery.

The desired babeld behavior is simpler: start with zero managed interfaces,
open the local control socket, emit normal local-control headers, and accept
later `interface <ifname>` commands.

## Existing Babeld Flow

Startup enters `babel_main(argv + optind, argc - optind)`.  The function:

- initialises kernel state;
- finalises configuration;
- adds positional interface arguments with `add_interface(...)`;
- previously failed if `interfaces == NULL`;
- opens the protocol socket and local control socket;
- enters the normal event loop.

The local-control `interface` command already works after configuration is
finalised: parsed interface configuration reaches `add_ifconf(...)`, and that
function calls `add_interface(...)` when `config_finalised` is true.

## New Behavior

An empty managed-interface list is valid at startup.  babeld still performs the
same setup work and enters the same event loop.  Interface-dependent work is
already guarded by interface iteration or `if_up(...)` checks, so an empty
interface list is naturally idle.

The daemon can later be populated with:

```text
interface <ifname>
```

over the read-write local control socket.

## Non-Goals

- Do not change the Babel wire protocol.
- Do not auto-discover or auto-manage all system interfaces.
- Do not add a new command-line option; zero positional interfaces is now a
  valid mode.
- Do not modify `babblerd` in this branch.

## Footguns

- Starting with no interfaces and no usable read-write local control socket is
  operationally inert.  The process can still run, but nothing can add managed
  links except config reload/control paths already present in babeld.
- `interface <ifname>` still requires the same privileges and kernel/socket
  capabilities as before.  This change only removes the startup-time
  requirement.
- Monitor clients should expect an initial snapshot with no interface lines
  until interfaces are added.
