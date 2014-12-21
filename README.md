# fake-ipmistack

fake-ipmistack is a simple and naive implementation for IPMItool functionality
testing.

## Installation

There is no installation per se. There is no point in having one.

```sh
git clone <repo_url> fake-ipmistack
cd fake-ipmistack
mkdir build
cd build
cmake ../ && make VERBOSE=1
# if compiled successfully
./src/fake-ipmistack
[INFO] server is awaiting new connection
```

## Directory structure

```
├── include
│   └── fake-ipmistack - header files
├── lib - modules/functional parts and helpers
└── src - top-level/apps(?)
```

## Interface

In case you'd like to use fake-ipmistack outside of IPMItool, you're probably
wondering about its interface. It's using UNIX sockets to communicate with
client as it was the simplest and the quickest way to do it. Feel free to
check-out IPMItool's "dummy" interface to get the idea how to implement
fake-ipmistack compatible interface in your project.

Structures used to shift data back and forth are in
``./include/fake-ipmistack/fake-ipmistack.h``:
 - ``dummy_rq`` represents client's request
 - ``dummy_rs`` represents reponse sent back to client
