# Fuzzy PSI

## How to Build

```bash
makedir build && cd build
cmake ..
make -j
```


## Usage Guide for `./build/fpsi`

This section describes the usage of the executable file located at `./build/main`.

### Command Flags

| Flag | Meaning             | Optional Values                                |
|:----:|:-------------------:|----------------------------------------------|
| d    | Dimension           |  |
| m    | Metric              | `0`: $L_\infty$, `1`: $L_1$, `2`: $L_2$ |
| delta| $\delta$              | Should be power of 2  |
| nn    | The logarithm of the input set size | Tested values: `4`, `8`(default), `12` (only supports balanced case) |
| v | Verbose | `0`: off, `1`: info(default), `2`: debug, `3`: error |
| try | Number of executions | 1 (default value) |
| prefix | Prefix optimization | 0 (default value) |

### Usage samples

```bash
# fuzzy PSI 
./fpsi -nn 8 -d 8 -delta 16 -p 0 -v 1

# fuzzy PSI with prefix optimization
./fpsi -nn 8 -d 8 -delta 16 -p 0 -v 1 -prefix
```