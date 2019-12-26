# netcat
Simple netcat written in C. Supports ip4, ip6 and hostnames. Client and server mode, tpc and udp available.

## Compilation
```bash
git clone https://github.com/guzlewski/netcat
cd netcat
make
```

## Usage

```
./netcat.out [OPTIONS] [DESTINATION] [PORT]

  -u - use udp instead of tcp
  -l - server mode, DESTINATION can be blank, defalut ip will be used
  -4 - force ip4
  -6 - force ip6
```
[DESTINATION] - can be ip4, ip6 or hostname (will be resolved via DNS)  
[PORT] - number

## License
Copyright (c) Michał Guźlewski. All rights reserved.
