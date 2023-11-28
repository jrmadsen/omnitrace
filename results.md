# Benchmark results

## Baseline

```shell
$ ./lulesh -s 220 -p -r 17 -c 76

Elapsed time         =      55.09 (s)
Grind time (us/z/c)  = 0.083443099 (per dom)  (0.083443099 overall)
FOM                  =  11984.215 (z/s)
```

## rocprofv1

```shell
$ rocprof --hip-trace --roctx-trace --tool-version 1 --parallel-kernels -d rocprof-v1 -o rocprof-v1/run.csv ./lulesh -s 220 -p -r 17 -c 76

Elapsed time         =      56.39 (s)
Grind time (us/z/c)  = 0.085411455 (per dom)  (0.085411455 overall)
FOM                  =  11708.031 (z/s)
```

## rocprofv2

```shell
$ rocprofv2 --hip-trace --roctx-trace -d rocprof-v2 -o run ./lulesh -s 220 -p -r 17 -c 76

Elapsed time         =      56.16 (s)
Grind time (us/z/c)  = 0.085068632 (per dom)  (0.085068632 overall)
FOM                  =  11755.214 (z/s)
```

## rocprofv3

```shell
$ rocprofv3 --hip-trace --marker-trace --kernel-trace -d rocprof-v3 -o run -- ./lulesh -s 220 -p -r 17 -c 76

Elapsed time         =      56.17 (s)
Grind time (us/z/c)  = 0.085078888 (per dom)  (0.085078888 overall)
FOM                  =  11753.797 (z/s)
```
