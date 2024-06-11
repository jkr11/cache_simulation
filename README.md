# gra_project_test

## How to use
Put this in the workspace you used for all other artemis projets, so that your systemc file is besides this respository
```bash
git clone https://github.com/jkr11/gra_project_test.git
git checkout -b <personal_branch>
# to push
git add <file>
git commit -m "<message>"
git push origin HEAD
```
then open a pull request and merge it.

We should also make sure to test everything here and then everyone commits their own files to Artemis so we can track who did what (organization will check for this).

## TODOS

### codestyle

would just follow the ÜL here, so pascalCase for variables and snake_case for functions 

means i have to refactor some stuff but thats ok

for def all upper snake case 
```C
#define HANDLE_ERROR(msg) ...
```

### csv and cli parsing

both of these should work now

actually "if tracefile is NULL no tracefile shoud be created" so this should probably be done in main.c

### Makefile

this might be real hard, as the interop between C <-> C++ <-> systemC is a lot of different paths

also everything needs to be commented

### systemC part

should actually be conceptually easy, but hard to debug

i would propose: 

```C++
SC_MODULE(CACHE) //this will be L1 and L2
                 //needs write, read in and hit out

SC_MODULE(MEMORY) // fallback for when both miss

SC_MODULE(CACHE_CONTROLLER) //ma`ages both L1 and L2 and Memory
```
Reading further, as far as I understand that both storage (memory) and the replacement strategy should be implemented using gates, (maybe we should ask out tutor here).

But what we could do now is:

```C++
SC_MODULE(Cacheline) { ... }

SC_MODULE(Cache) {
  ...
  CacheLine **cachelines;
  sc_vector<sc_signal<sc_uint<8>>> lru;
}
```
for (relatively) easy LRU replacement.
 
### other

feel free to add things like utils for printing any structs and other things

maybe write logging for systemC (i dont know if this is possible)