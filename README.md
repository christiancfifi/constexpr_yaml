# Constexpr Yaml
[![build and test](https://github.com/christiancfifi/constexpr_yaml/actions/workflows/ci.yml/badge.svg)](https://github.com/christiancfifi/constexpr_yaml/actions/workflows/ci.yml)

A single header C++26 compile-time yaml parser, using reflection. Try it on [godbolt](https://godbolt.org/z/bhzGz5s59).

### Usage 
```cpp
constexpr auto parsed = constexpr_yaml::parse<R"(
example:
  value: 0x10
)">;
static_assert(parsed.example.value == 0x10);
```
### Parse an #embed-ed  file
```cpp
constexpr const char yaml[] = {
    #embed "test.yaml"
    , 0
};
constexpr auto parsed = constexpr_yaml::parse<yaml>;
```
## Note on installation
At the time of writing, Bloomberg's [clang-p2996](https://github.com/bloomberg/clang-p2996/blob/p2996/P2996.md) fork was the only viable option for reflection support. Since then, GCC 16 has been released with support and is likely your best bet. Nevertheless, if you compile clang-p2996 from source you can use the included xmake build file to build without installing a janky clang to your system:
```bash
xmake f -c --clang_p2996_dir=~/src/clang-p2996/build
xmake
xmake test
```
[xmake](https://github.com/xmake-io/xmake) is great, by the way. It's an all in one build system like a CMake + Ninja + Ccache + Conan, but you write lua instead of DSLs.
## Yaml 1.2 Support Matrix
Presently, this is a short and sweet (~) ad-hoc parser that implements basic mapping of keys to values or sequences of values. In the future I would like to see a generic reflection-based parser generator that allows this to be migrated to a more generic lexer/recursive descent parser structure. Then, the implementation can be largely taken from an existing fully featured runtime parser.

Nodes
- [x] Maps
- [x] Sequences
- [ ] Inline maps
- [x] Inline sequences

Literals
- [x] Boolean
- [x] Decimal integers
- [x] Octal
- [x] Hex
- [x] Floats
- [ ] Sexagesimal
- [ ] Timestamps
- [x] Null
- [x] Strings
- [ ] Empty Maps
- [ ] Empty Collections

Advanced
- [x] Comments (between members) 
- [x] Unicode
- [x] Escaped characters (sans unicode)
- [ ] Documents
- [ ] Anchors
- [ ] Aliases
- [ ] Complex mapping keys
- [ ] Literal block scalars
- [ ] Chomping indicators
- [ ] indentation indicators
- [ ] YAML version directive
- [ ] Tags
- [ ] JSON flow style
- [ ] Properties
