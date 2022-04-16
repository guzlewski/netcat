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
./netcat.out [OPTIONS] [ADDRESS] [PORT]

  -u - use udp instead of tcp
  -l - server mode, ADDRESS can be blank, defalut ip will be used
  -4 - force ip4
  -6 - force ip6
```

`ADDRESS` - ip4, ip6 or hostname (will be resolved via DNS) to connect or bind in server mode  
`PORT` - number of port to connect or listen
