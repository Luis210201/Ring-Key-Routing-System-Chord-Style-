# Ring Key Routing System (Chord-Style)

A C implementation of a ring-based key routing system.  
This project simulates a small distributed database where nodes are arranged in a logical ring and exchange messages over TCP and UDP sockets. Each node stores a subset of keys, routes lookup requests, and can dynamically join or leave the ring.

---

## Key Features

### Key Lookup
Nodes collaboratively route lookup messages around the ring, forwarding requests through successor links or optional chord shortcuts to reduce lookup hops.

### Node Join
Nodes can join the ring in two ways:
- **pentry** — entering with knowledge of the predecessor  
- **bentry** — entering without knowing the correct position, discovering it via lookup

### Node Leave
A departing node transfers ownership of its stored keys to its predecessor and informs its neighbors to update routing pointers.

### 🔗 Chord-Style Shortcuts
Nodes may maintain an optional shortcut link to accelerate routing.

### Hybrid Communication Model
- **TCP:** successor / predecessor communication, join and leave coordination  
- **UDP:** chord routing and join-position discovery

---

## Architecture Overview

Each node runs as a standalone process maintaining:

- Its own **key**
- **Successor** and **predecessor**
- Optional **shortcut**
- Set of stored keys

Nodes communicate using a simple message protocol and implement the required functionalities:
- Key lookup (`find`)
- Join and leave procedures
- Shortcut management (`chord` / `echord`)
- Predecessor discovery (UDP)

---

## Commands Implemented

| Command | Description |
|--------|-------------|
| `new` | Creates a new ring with a single node |
| `pentry pred pred.IP pred.port` | Join ring with known predecessor |
| `bentry boot boot.IP boot.port` | Join ring without knowing ring position |
| `find k` | Lookup key `k` |
| `chord i i.IP i.port` | Add a chord shortcut to node `i` |
| `echord` | Remove the current chord shortcut |
| `show` | Display node state |
| `leave` | Exit the ring gracefully |
| `exit` | Terminate the program |

---

## Build

```bash
make
```

- This produces the the executable: ring

---

## Run

```bash
./ring <key> <ip> <port>
```

