# LINGI1341-linksim
A link simulator for the first networking project (LINGI1341)

This program will proxy UDP traffic (datagrams of max 520 bytes), between
two hosts, simulating the behavior of a lossy link.

Using it is a simple as choosing two UDP ports, one for the proxy, and one for the receiver of the protocol.

Client machine:

```bash
./sender server_address -proxy_port
```

Server machine:
```bash
./link_sim -p proxy_port -P server_port &
./receiver :: server_port
```

The program only alter the _forward_  direction of the data transfert, traffic from the
receiver to the sender will be left untouched.

Use this to test your programs in the events of losses, delays, ...

Feel free to hack it and submit pull request for bug fixes or new features,
e.g. to also affect reverse traffic (e.g. with an optional command-line switch)
