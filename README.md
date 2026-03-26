# Fuzzy PSI

This repository provides the implementation and build scripts for fuzzy private set intersection.

> Note: This project is experimental and primarily intended for research use. Adjust parameters according to your hardware and dataset sizes.


## Source Code Location of Main Functionality
- `src/*.cpp` contains the implementations of our building blocks such as `si-OPRF`, `so-OPRF`, `so-OPPRF` and other MPC components
- `src/fpsi.cpp` contains the implementations of basic `fuzzy mapping`, `fuzzy PSI` protocol
- `src/fpsi_prefix.cpp` contains the implementations of **prefix-optimized** `fuzzy mapping`, `fuzzy PSI` protocol

## Requirements

- `cmake`, `make`, `g++ 13`
- Docker (optional, for isolated builds)
- Additional third-party libraries [secure-join](https://github.com/Visa-Research/secure-join.git) and [volePSI](https://github.com/ladnir/volepsi.git) (can be installed by the script [build.sh](./build.sh))


- **Dependencies :**

```bash
build-essential
cmake
git 
libtool 
iproute2 
python3 
sudo 
nasm 
libssl-dev 
libgmp-dev 
wget 
libfmt-dev
```

## Local build

From the project root directory:

```bash
./build.sh    # installs third-party dependencies if needed (about 10-15 mins)
mkdir -p build && cd build
cmake ..
make -j

# The executable will be located at ./build/fpsi
```

## Docker (optional)

Use Docker for an isolated or reproducible build environment:

```bash
docker build -t <your-image-name> .
docker run -it --name <your-container-name> --cap-add=NET_ADMIN --memory=512g <your-image-name>
docker exec -it <your-container-name> bash
```

## Executable: `./build/fpsi`

Below are the commonly used command-line flags. Flags use a leading dash (for example `-nn`, `-d`).

| Flag | Meaning | Values / Notes |
|---|---|---|
| `-d` | Dimension | integer |
| `-m` | Metric | `0`: $L_\infty$, `1`: $L_1$, `2`: $L_2$ |
| `-delta` | Distance threshold (δ) | recommended to be a power of 2 |
| `-nn` | log2 of input set size (n) | tested values: `8`~`16` |
| `-v` | Verbosity | `0`: off (default), `1`: info |
| `-try` | Number of runs | integer, default `1` |
| `-prefix` | Prefix optimization flag | `0`: off (default), `1`: on |

## Usage examples

Run a basic fuzzy PSI experiment:

```bash
./fpsi -nn 8 -d 8 -delta 16 -v 1
```

Enable prefix optimization:

```bash
./fpsi -nn 8 -d 8 -delta 16 -v 1 -prefix 1
```

## Benchmark Instructions

We provide two scripts to reproduce the benchmark results reported in
the paper.

### Table 2 Results

To reproduce the results in **Table 2**, run:

``` bash
./bench1.sh
```

from the root directory of this repository.

The script produces output similar to (the results are averaged for 5 repeated run):

    [ProType] [Metric] [Dim] [Delta] [Size] [Com.(MB)] [Time(s)]
    [ours]    L0      4     16     256     7.343      0.782
    [ours]    L1      4     16     256     7.802      0.916
    [ours]    L2      4     16     256     7.839      1.009

where:

-   **L0 / L1 / L2** correspond to the metrics $L_\infty$, $L_1$, and
    $L_2$
-   **Dim** corresponds to parameter $d$
-   **Delta** corresponds to parameter $\delta$
-   **Size** corresponds to the input set size $n$
------------------------------------------------------------------------

### Table 3 Results

To reproduce the results in **Table 3**, run:

``` bash
./bench2.sh
```

from the root directory of this repository.

------------------------------------------------------------------------

## Baseline Implementations

The following baseline implementations are used for comparison.

### Gao et al.[14]

Code repository:

https://github.com/ql70ql70/Fuzzy-Private-Set-Intersection-from-Fuzzy-Mapping

Recommended Docker image:

    blueobsidian/gao_artifact:latest

------------------------------------------------------------------------

### Dang et al.[16]

Code repository:

https://github.com/zhouxv/ourFuzzyPSI-C

Recommended Docker image:

    blueobsidian/fpsi_artifact:latest

------------------------------------------------------------------------

## Research Claims

Our paper proposes two main constructions:

1.  **Basic Fuzzy PSI** (without prefix optimization)
2.  **Prefix Fuzzy PSI**

------------------------------------------------------------------------

### Basic Fuzzy PSI

For the **basic fuzzy PSI construction**, both the running time and
communication cost grow **linearly** with:

-   the input size ($n$)
-   the dimension ($d$)
-   the distance threshold ($\delta$)

This linear trend becomes observable when the parameters are
sufficiently large.\
For example, when $n > 2^{{12}}$, the setup cost becomes negligible
compared to the protocol cost. For very small inputs ($2^8$), the setup cost
dominates and may lead to non-linear behavior.

Compared with prior work [14,16], we expect:

-   **3--19× reduction in communication cost**
-   **9--140× reduction in running time**

Note that communication cost is independent of the hardware platform,
while running time may be affected by CPU frequency, cache size, and
other hardware factors.

------------------------------------------------------------------------

### Prefix Fuzzy PSI

For **fuzzy PSI with prefix optimization**, the running time and
communication cost grow linearly with:

-   the input size ($n$)
-   the dimension ($d$)
-   **the logarithm of the distance threshold ($\log \delta$)**

As with the basic construction, this behavior becomes observable when
parameters are sufficiently large (e.g., $n > 2^{{12}}$). For smaller
inputs ($2^8$), setup costs may dominate.

In addition, the prefix optimization requires that **$\delta$ be a power
of two**, as described in the paper.

Compared with the state-of-the-art work of **Dang et al.**[16], our
implementation achieves approximately:

-   **7--10× improvement in communication efficiency**
-   **13--38× improvement in computational efficiency**

Note that communication cost is independent of the hardware platform, while
running time may vary depending on CPU parameters such as frequency
and cache size.


### Note

The running time of the baseline implementations can be quite large for certain parameter settings. 
Evaluating all parameter combinations may take more than **1 day** to complete.

We recommend selecting appropriate parameters for experiments according to the computational capability of your hardware platform.



## Citation

If you make use of our work, please consider citing us:

```bibtex
@INPROCEEDINGS {
    author = { Xinpeng, Yang and Meng, Hao and Chenkai, Weng and Robert H., Deng and Yonggang, Wen and Tianwei, Zhang },
    booktitle = { 2026 IEEE Symposium on Security and Privacy (SP) },
    title = {{ Efficient Fuzzy Private Set Intersection from Secret-shared OPRF }},
    year = {2026},
}
```