# Open Screen Protocol Demos

This directory contains demonstration programs for the Open Screen Protocol implementation.

## Available Demos

### OSP Demo (`osp_demo`)
A comprehensive demonstration of the Presentation API controller and receiver functionality. Shows how to:
- Discover available receivers via mDNS
- Start and manage presentations 
- Send bidirectional messages
- Handle connection lifecycle

**Location**: `osp/demo/osp_demo/`  
**Build**: `ninja -C out/debug osp_demo`

### OSP Network Demo (`osp_network_demo`)  
A minimal example demonstrating basic protocol connection setup and messaging. Shows how to:
- Set up server and client connections
- Perform automatic service discovery
- Exchange simple messages over the protocol

**Location**: `osp/demo/osp_network_demo/`  
**Build**: `ninja -C out/debug osp_network_demo`

## Building All Demos

From the repository root:

```bash
gn gen out/debug
ninja -C out/debug osp/demo:osp_demo osp/demo:osp_network_demo
```

Or build everything including demos:

```bash
ninja -C out/debug gn_all
```

## See Also

- [OSP Documentation](../README.md)
- [Threading Guide](../../docs/threading.md)
- [Style Guide](../../docs/style_guide.md)