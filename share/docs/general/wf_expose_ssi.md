# expose: tcp/udp port forwarding from master host to jail

## command: expose

```sh
cbsd expose jname=test2 mode=add in=200 out=200
cbsd expose jname=test2 mode=delete in=200 out=200
cbsd expose jname=test2 mode=list
cbsd expose jname=test2 mode=clear
cbsd expose jname=test2 mode=flush
```

By command **cbsd expose** you can create forward rule for tcp/udp port from external IP to jail.

Copyright © 2013—2024 CBSD Team.

