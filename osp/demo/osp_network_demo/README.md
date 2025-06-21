# OSP Network Demo - Protocol Connection Example

This directory contains a minimal example for demonstrating automatic discovery
of an Open Screen protocol connection server and client using the base
DNS-SD service listener and publisher. There are no command-line options;
the server and client modes are selected via a single `server` or `client`
argument.

## Building and Running

First, generate and build the example from the repo root:

```bash
gn gen out/debug
ninja -C out/debug osp_network_demo
```

## Running the server

Then start the server:

```bash
out/debug/osp_network_demo server
```

The server listens on the loopback interface on port 9988, advertises a
DNS‑SD service with instance name `SimpleConnectionExample`, and prints
incoming connection instance and connection IDs. Press `Ctrl-C` to stop.

## Running the client

In another terminal, run the client to connect to the server:

```bash
out/debug/osp_network_demo client
```

The client discovers the server automatically via mDNS, connects to the first
available instance named `SimpleConnectionExample`, sends a "Hello from client!"
message to the server, receives an echo response back ("ECHO: Hello from client!"),
and reports success or failure before exiting.

## Message Flow

1. Client connects to server via mDNS discovery
2. Client sends: `"Hello from client!"`
3. Server receives message and responds with: `"ECHO: Hello from client!"`
4. Client receives and displays the echo response
5. Both client and server terminate cleanly

The example demonstrates bidirectional messaging over Open Screen Protocol connections.