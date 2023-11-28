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
$ rocprof --sys-trace --tool-version 1 --parallel-kernels -d rocprof-v1 -o rocprof-v1/run.csv ./lulesh -s 220 -p -r 17 -c 76

Elapsed time         =      56.71 (s)
Grind time (us/z/c)  = 0.08589652 (per dom)  (0.08589652 overall)
FOM                  =  11641.915 (z/s)
```

## rocprofv2

```shell
$ rocprofv2 --sys-trace -d rocprof-v2 -o run ./lulesh -s 220 -p -r 17 -c 76

Elapsed time         =      56.94 (s)
Grind time (us/z/c)  = 0.086252216 (per dom)  (0.086252216 overall)
FOM                  =  11593.905 (z/s)
```

## rocprofv3

```shell
$ rocprofv3 --sys-trace -d rocprof-v3 -o run -- ./lulesh -s 220 -p -r 17 -c 76

Elapsed time         =      68.38 (s)
Grind time (us/z/c)  = 0.10357751 (per dom)  (0.10357751 overall)
FOM                  =  9654.6056 (z/s)
```
