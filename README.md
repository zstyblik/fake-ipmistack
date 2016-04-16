# fake-ipmistack

fake-ipmistack is a simple and naive implementation of IPMI stack/BMC according
to Intel's [IPMI specification] for functional testing.

Its main purpose is to test [IPMItool]. However, it can be used for testing of
other IPMI CLI tools and applications.

## Compilation

```sh
mkdir build
cd build
cmake ../ && make VERBOSE=1
# if compiled successfully
./src/fake-ipmistack
[INFO] server is awaiting new connection
```

Fake-ipmistack is now listening for requests via UNIX socket, `/tmp/.ipmi_dummy`.

## Directory structure

```
├── include
│   └── fake-ipmistack - header files
├── lib - modules/functional parts and helpers
└── src - top-level/apps(?)
```

## Interface

In case you'd like to use fake-ipmistack outside of IPMItool, you're probably
wondering about its interface. It's using UNIX socket to communicate with
client as it was the simplest and the quickest way to do it. Feel free to
check-out IPMItool's [dummy interface] to get the idea how to implement
fake-ipmistack compatible interface in your project.

Structures used to shift data back and forth are in
`./include/fake-ipmistack/fake-ipmistack.h`:
 - `dummy_rq` represents client's request
 - `dummy_rs` represents reponse sent back to client

[dummy interface]: https://sourceforge.net/p/ipmitool/source/ci/master/tree/src/plugins/dummy/
[IPMItool]: https://sourceforge.net/p/ipmitool/
[IPMI specification]: http://www.intel.com/content/www/us/en/servers/ipmi/ipmi-second-gen-interface-spec-v2-rev1-1.html
