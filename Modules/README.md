Multi Architecture Support
========
If you are using a 64bit architecture, see https://wiki.debian.org/Multiarch/HOWTO
to enable multi-arch support. CFS builds 32 bit binaries and hence you will have to enable
multi-arch support. 

## `-lwrap` library not found

- Use
```
find / -name libwrap*
```
to check if the tcp wrapper library is available. It is typically
located under `/lib/x86_64-linux-gnu` or
`i386-linux-gnu`.
- If `libwrap` is not available, install it:
```
sudo apt-get install tcpd tcpd:i386
```

- Make sure `libwrap.so` is symbolically linked to `libwrap.so.x`,
  which in turn is a symbolic link to the actual library `libwrap.so.x.x.x`.

### Multilib
- Make sure g++multlib, gcc-multilib are installed/updated.
  
