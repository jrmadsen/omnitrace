# OmniTrace Output

```eval_rst
.. toctree::
   :glob:
   :maxdepth: 3
```

## Overview

The general output form of omnitrace is `<OUTPUT_PATH>[/<TIMESTAMP>]/[<PREFIX>]<DATA_NAME>[-<OUTPUT_SUFFIX>].<EXT>`.

E.g. with the base configuration:

```shell
export OMNITRACE_OUTPUT_PATH=omnitrace-example-output
export OMNITRACE_TIME_OUTPUT=ON
export OMNITRACE_USE_PID=OFF
export OMNITRACE_USE_TIMEMORY=ON
export OMNITRACE_USE_PERFETTO=ON
```

```shell
$ omnitrace-instrument -- ./foo
...
[omnitrace] Outputting 'omnitrace-example-output/perfetto-trace.proto'...

[omnitrace] Outputting 'omnitrace-example-output/wall-clock.txt'...
[omnitrace] Outputting 'omnitrace-example-output/wall-clock.json'...
```

If we enable the `OMNITRACE_USE_PID` option, then when our non-MPI executable is executed with a PID of 63453:

```shell
$ export OMNITRACE_USE_PID=ON
$ omnitrace-instrument -- ./foo
...
[omnitrace] Outputting 'omnitrace-example-output/perfetto-trace-63453.proto'...

[omnitrace] Outputting 'omnitrace-example-output/wall-clock-63453.txt'...
[omnitrace] Outputting 'omnitrace-example-output/wall-clock-63453.json'...
```

If we enable `OMNITRACE_TIME_OUTPUT`, then a job started on January 31, 2022 at 12:30 PM:

```shell
$ export OMNITRACE_TIME_OUTPUT=ON
$ omnitrace-instrument -- ./foo
...
[omnitrace] Outputting 'omnitrace-example-output/2022-01-31_12.30_PM/perfetto-trace-63453.proto'...

[omnitrace] Outputting 'omnitrace-example-output/2022-01-31_12.30_PM/wall-clock-63453.txt'...
[omnitrace] Outputting 'omnitrace-example-output/2022-01-31_12.30_PM/wall-clock-63453.json'...
```

## Metadata

[OmniTrace](https://github.com/AMDResearch/omnitrace) will output a metadata.json file. This metadata file will contain
information about the settings, environment variables, output files, and info about the system and the run:

- Hardware cache sizes
- Physical CPUs
- Hardware concurrency
- CPU model, frequency, vendor, and features
- Launch date and time
- Memory maps (e.g. shared libraries)
- Output files
- Environment Variables
- Configuration Settings

### Metadata JSON Sample

```json
{
    "omnitrace": {
        "metadata": {
            "info": {
                "HW_L1_CACHE_SIZE": 32768,
                "HW_L2_CACHE_SIZE": 524288,
                "HW_L3_CACHE_SIZE": 16777216,
                "HW_PHYSICAL_CPU": 12,
                "HW_CONCURRENCY": 24,
                "LAUNCH_TIME": "02:04",
                "LAUNCH_DATE": "05/08/22",
                "TIMEMORY_GIT_REVISION": "52e7034fd419ff296506cdef43084f6071dbaba1",
                "TIMEMORY_VERSION": "3.3.0rc4",
                "TIMEMORY_API": "tim::project::timemory",
                "TIMEMORY_GIT_DESCRIBE": "v3.2.0-263-g52e7034f",
                "PWD": "/home/jrmadsen/devel/c++/AARInternal/hosttrace-dyninst/build-vscode",
                "USER": "jrmadsen",
                "HOME": "/home/jrmadsen",
                "SHELL": "/bin/bash",
                "CPU_MODEL": "AMD Ryzen Threadripper PRO 3945WX 12-Cores",
                "CPU_FREQUENCY": 2400,
                "CPU_VENDOR": "AuthenticAMD",
                "CPU_FEATURES": [
                    "fpu",
                    "msr",
                    "sse",
                    "sse2",
                    "constant_tsc",
                    "ssse3",
                    "fma",
                    "sse4_1",
                    "sse4_2",
                    "popcnt",
                    "avx2",
                    "... etc. ..."
                ],
                "memory_maps": [
                    {
                        "end_address": "7f4013797000",
                        "start_address": "7f4012e58000",
                        "pathname": "/opt/rocm-5.0.0/hip/lib/libamdhip64.so.5.0.50000",
                        "offset": "34a000",
                        "device": "103:05",
                        "inode": 4331165,
                        "permissions": "rw-p"
                    },
                    {
                        "end_address": "7f4013902000",
                        "start_address": "7f4013901000",
                        "pathname": "/usr/lib/x86_64-linux-gnu/libm-2.31.so",
                        "offset": "14d000",
                        "device": "103:05",
                        "inode": 42078854,
                        "permissions": "rwxp"
                    },
                    {
                        "end_address": "7f4013919000",
                        "start_address": "7f4013908000",
                        "pathname": "/usr/lib/x86_64-linux-gnu/libpthread-2.31.so",
                        "offset": "6000",
                        "device": "103:05",
                        "inode": 42078874,
                        "permissions": "r-xp"
                    },
                    {
                        "...": "etc."
                    },
                ],
                "memory_maps_files": [
                    "/opt/rocm-5.0.0/hip/lib/libamdhip64.so.5.0.50000",
                    "/opt/rocm-5.0.0/hsa-amd-aqlprofile/lib/libhsa-amd-aqlprofile64.so.1.0.50000",
                    "/opt/rocm-5.0.0/lib/libamd_comgr.so.2.4.50000",
                    "/opt/rocm-5.0.0/lib/libhsa-runtime64.so.1.5.50000",
                    "/opt/rocm-5.0.0/rocm_smi/lib/librocm_smi64.so.5.0.50000",
                    "/opt/rocm-5.0.0/roctracer/lib/libroctracer64.so.1.0.50000",
                    "/usr/lib/x86_64-linux-gnu/ld-2.31.so",
                    "/usr/lib/x86_64-linux-gnu/libc-2.31.so",
                    "/usr/lib/x86_64-linux-gnu/libdl-2.31.so",
                    "... etc. ..."
                ],
            },
            "output": {
                "text": [
                    {
                        "value": [
                            "omnitrace-tests-output/parallel-overhead-binary-rewrite/roctracer.txt"
                        ],
                        "key": "roctracer"
                    },
                    {
                        "value": [
                            "omnitrace-tests-output/parallel-overhead-binary-rewrite/wall_clock.txt"
                        ],
                        "key": "wall_clock"
                    }
                ],
                "json": [
                    {
                        "value": [
                            "omnitrace-tests-output/parallel-overhead-binary-rewrite/roctracer.json",
                            "omnitrace-tests-output/parallel-overhead-binary-rewrite/roctracer.tree.json"
                        ],
                        "key": "roctracer"
                    },
                    {
                        "value": [
                            "omnitrace-tests-output/parallel-overhead-binary-rewrite/wall_clock.json",
                            "omnitrace-tests-output/parallel-overhead-binary-rewrite/wall_clock.tree.json"
                        ],
                        "key": "wall_clock"
                    }
                ]
            },
            "environment": [
                {
                    "value": "/home/jrmadsen",
                    "key": "HOME"
                },
                {
                    "value": "/bin/bash",
                    "key": "SHELL"
                },
                {
                    "value": "jrmadsen",
                    "key": "USER"
                },
                {
                    "value": "true",
                    "key": "... etc. ..."
                }
            ],
            "settings": {
                "OMNITRACE_JSON_OUTPUT": {
                    "count": -1,
                    "environ_updated": false,
                    "name": "json_output",
                    "data_type": "bool",
                    "initial": true,
                    "enabled": true,
                    "value": true,
                    "max_count": 1,
                    "cmdline": [
                        "--omnitrace-json-output"
                    ],
                    "environ": "OMNITRACE_JSON_OUTPUT",
                    "config_updated": false,
                    "categories": [
                        "io",
                        "json",
                        "native"
                    ],
                    "description": "Write json output files"
                },
                "... etc. ...": {
                    "etc.": true
                }
            }
        }
    }
}
```

## Configuring Output

### Core Configuration Settings

> ***See also: [Customizing OmniTrace Runtime](runtime.md)***

| Setting                   | Value              | Description                                                                                       |
|---------------------------|--------------------|---------------------------------------------------------------------------------------------------|
| `OMNITRACE_OUTPUT_PATH`   | Any valid path     | Path to folder where output files should be placed                                                |
| `OMNITRACE_OUTPUT_PREFIX` | String             | Useful for multiple runs with different arguments. See [Output Prefix Keys](#output-prefix-keys)  |
| `OMNITRACE_OUTPUT_FILE`   | Any valid filepath | Specific location for perfetto output file.                                                       |
| `OMNITRACE_TIME_OUTPUT`   | Boolean            | Place all output in a timestamped folder, timestamp format controlled via `OMNITRACE_TIME_FORMAT` |
| `OMNITRACE_TIME_FORMAT`   | String             | See `strftime` man pages for valid identifiers                                                    |
| `OMNITRACE_USE_PID`       | Boolean            | Append either the PID or the MPI rank to all output files (before the extension)                  |

#### Output Prefix Keys

Output prefix keys have many uses but most useful when dealing with multiple profiling runs or large MPI jobs.
Their inclusion in omnitrace stems from their introduction into timemory for [compile-time-perf](https://github.com/jrmadsen/compile-time-perf)
which needed to be able to create different output files for a generic wrapper around compilation commands while still
overwriting the output from the last time a file was compiled.

If you are ever doing scaling studies and specifying options via the command line, it is highly recommend to just
use a common `OMNITRACE_OUTPUT_PATH`, disable `OMNITRACE_TIME_OUTPUT`,
set `OMNITRACE_OUTPUT_PREFIX="%argt%-"` and let omnitrace cleanly organize the output.

| String          | Encoding                                                                                                           |
|-----------------|--------------------------------------------------------------------------------------------------------------------|
| `%argv%`        | Entire command-line condensed into a single string                                                                 |
| `%argt%`        | Similar to `%argv%` except basename of first command line argument                                                 |
| `%args%`        | All command line arguments condensed into a single string                                                          |
| `%tag%`         | Basename of first command line argument                                                                            |
| `%arg<N>%`      | Command line argument at position `<N>` (zero indexed), e.g. `%arg0%` for first argument.                          |
| `%argv_hash%`   | MD5 sum of `%argv%`                                                                                                |
| `%argt_hash%`   | MD5 sum if `%argt%`                                                                                                |
| `%args_hash%`   | MD5 sum of `%args%`                                                                                                |
| `%tag_hash%`    | MD5 sum of `%tag%`                                                                                                 |
| `%arg<N>_hash%` | MD5 sum of `%arg<N>%`                                                                                              |
| `%pid%`         | Process identifier (i.e. `getpid()`)                                                                               |
| `%ppid%`        | Parent process identifier (i.e. `getppid()`)                                                                       |
| `%pgid%`        | Process group identifier (i.e. `getpgid(getpid())`)                                                                |
| `%psid%`        | Process session identifier  (i.e. `getsid(getpid())`)                                                              |
| `%psize%`       | Number of sibling process (from reading `/proc/<PPID>/tasks/<PPID>/children`)                                      |
| `%job%`         | Value of `SLURM_JOB_ID` environment variable if exists, else `0`                                                   |
| `%rank%`        | Value of `SLURM_PROCID` environment variable if exists, else `MPI_Comm_rank` (or `0` non-mpi)                      |
| `%size%`        | `MPI_Comm_size` or `1` if non-mpi                                                                                  |
| `%nid%`         | `%rank%` if possible, otherwise `%pid%`                                                                            |
| `%launch_time%` | Launch date and time (uses `OMNITRACE_TIME_FORMAT`)                                                                |
| `%env{NAME}%`   | Value of environment variable `NAME` (i.e. `getenv(NAME)`)                                                         |
| `%cfg{NAME}%`   | Value of configuration variable `NAME` (e.g. `%cfg{OMNITRACE_SAMPLING_FREQ}%` would resolve to sampling frequency) |
| `$env{NAME}`    | Alternative syntax to `%env{NAME}%`                                                                                |
| `$cfg{NAME}`    | Alternative syntax to `%cfg{NAME}%`                                                                                |
| `%m`            | Shorthand for `%argt_hash%`                                                                                        |
| `%p`            | Shorthand for `%pid%`                                                                                              |
| `%j`            | Shorthand for `%job%`                                                                                              |
| `%r`            | Shorthand for `%rank%`                                                                                             |
| `%s`            | Shorthand for `%size%`                                                                                             |

> ***Any output prefix key which contain a `/` will have the `/` characters***
> ***replaced with `_` and any leading underscores will be stripped, e.g. if `%arg0%` is `/usr/bin/foo`, this***
> ***will translate to `usr_bin_foo`. Additionally, any `%arg<N>%` keys which do not have a command line argument***
> ***at position `<N>` will be ignored.***

## Perfetto Output

The perfetto backend generates a trace. Enable perfetto tracing output via `OMNITRACE_USE_PERFETTO=ON` or the
`-T`/`--trace` option to `omnitrace-run`.
By default, Perfetto traces are specific to a single OS process, for a single output file containing traces from
multiple processes, see the [Perfetto trace for multiple processes](#perfetto-trace-for-multiple-processes) section.
Use the `OMNITRACE_OUTPUT_FILE` to specify a specific output filename. If this is an absolute path, then the `OMNITRACE_OUTPUT_PATH`, etc.
settings will be ignored. Perfetto trace files generated by Omnitrace are binary protobuf files with a `.proto` filename extension,
although it is also common for these files to have the extension `.pftrace`.
Once Omnitrace has generated the protobuf file(s), visit [ui.perfetto.dev](https://ui.perfetto.dev) in any web browser and
select the "Open trace file" option from the panel on the left and navigate to the protobuf file. If the trace was generated
on a machine without a windowing system and/or a web browser, you can transfer the protobuf file from the remote machine to your
local machine (via tools such as `scp`, `rsync`, etc.) and open the trace file in the browser of your local machine. Be aware that
as the size of the Perfetto trace file starts to reach and exceed 500 MB, rendering might take a significant amount of time. Trace
files in the GB range will frequently result in the Perfetto visualizer prompting you to download and execute an external trace processor
application in the console/terminal (i.e. outside of the web browser) to work-around the (computational) resource limitations of the
visualizer within the web browser.

![omnitrace-perfetto](images/omnitrace-perfetto.png)

![omnitrace-rocm](images/omnitrace-rocm.png)

![omnitrace-rocm-flow](images/omnitrace-rocm-flow.png)

![omnitrace-user-api](images/omnitrace-user-api.png)

### Perfetto trace for multiple processes

By default, Omnitrace enables the `inprocess` backend for Perfetto and will generate one Perfetto protobuf trace file
per process. If the target application uses multi-processing, this default configuration can be problematic due
to (A) a desire to view the traces for each process in a single file or (B) missing trace information for the child
process. If either of these apply, please read the following section.

If your application creates child processes (via `fork()`), the `inprocess` backend for Perfetto is problematic:
Perfetto uses a background thread to process incoming trace data and when an application invokes
`fork()`, this background thread is not copied into the child process but all the infrastructure for handling the
thread is (mutexes, `std::thread` object, etc.).
In order for Omnitrace to prevent Perfetto from attempting to "join" this background thread within the child process (which
it does not own) and crashing the child process during finalization as a result, Omnitrace must disable Perfetto in the child process.
Thus, Omnitrace + Perfetto with the "inprocess" backend will not provide any insight into the performance of child processes.
In order to gain insight into the parent and child process, one must configure Perfetto to use the `system`
backend via `OMNITRACE_PERFETTO_BACKEND=system` in a configuration file or `--perfetto-backend=system` with `omnitrace-run`.

Perfetto tracing with the system backend supports multiple processes writing to the same
output file. Thus, it is a useful technique if Omnitrace is built with partial MPI support
because all the perfetto output will be coalesced into a single file. The
installation docs for perfetto can be found [here](https://perfetto.dev/docs/contributing/build-instructions).
If you are building omnitrace from source, you can configure CMake with `OMNITRACE_INSTALL_PERFETTO_TOOLS=ON`
and the `perfetto` and `traced` applications will be installed as part of the build process. However,
it should be noted that to prevent this option from accidentally overwriting an existing perfetto install,
all the perfetto executables installed by omnitrace are prefixed with `omnitrace-perfetto-`, except for the `perfetto`
executable, which is just renamed `omnitrace-perfetto`.

#### Perfetto System Backend Configuration

Omnitrace provides a sample Perfetto config file in `${OMNITRACE_ROOT}/share/omnitrace/perfetto.cfg`:

```console
duration_ms: 300000
write_into_file: true

buffers {
  size_kb: 1024
  fill_policy: DISCARD
}

data_sources {
  config {
      name: "track_event"
  }
}
```

If you are unsure of the application runtime, remove the `duration_ms` field and once the application has completed,
sent an "interrupt" signal to the Perfetto daemon. This can be done via two methods:

1. Use two console/terminal sessions on the same machine
    - Start the Perfetto daemon _**without**_ the `--background` flag in the second terminal (i.e. before launching the target application)
    - Launch the target application in the first terminal, e.g. `omnitrace-run --perfetto-backend=system -- <target-app>`
    - When the target application completes, navigate to the second terminal and hit `Ctrl+C` and the Perfetto daemon will exit and
      generate the output file
2. Use one terminal session
    - Start the Perfetto daemon with the `--background` flag and capture/record the PID printed by the daemon, e.g.,
      `PERFETTO_DAEMON_PID=$(omnitrace-perfetto --background <additional-args...>)`
    - Launch the target application, e.g. `omnitrace-run --perfetto-backend=system -- <target-app>`
    - When the target application completes, send the interrupt signal to the PID of the daemon, e.g.
      `kill -s SIGINT ${PERFETTO_DAEMON_PID}`

#### Perfetto System Backend Overview

Using Perfetto in system mode requires starting two applications before executing your application:
(1) `traced` and (2) `perfetto`. If using the Omnitrace Perfetto installation, these applications are
named `omnitrace-perfetto-traced` and `omnitrace-perfetto`, respectively. It is recommended to use the
`--background` flag when starting each of these applications. The `traced` application should be started
first; it generates two sockets (a producer socket and consumer socket) which are used to coordinate communication
between the `perfetto` application/daemon and the application generating Perfetto data via Omnitrace.
Once `traced` has started, the `perfetto` application is started and configured with field such as output filename,
duration to collect trace data, etc. Once these two applications have started, Omnitrace's Perfetto system backend
is ready to be used and can be enabled via `OMNITRACE_PERFETTO_BACKEND=system` (config) or `--perfetto-backend=system`
(command line).

#### Testing for Existing Perfetto Daemons

As a general rule of thumb, developers only want one instance of `traced` and `perfetto` to be running per machine
to prevent a proliferation of unused zombie `traced` and `perfetto` daemons, which can easily happen in extended
performance analysis sessions. One method to prevent this might be to execute `killall perfetto` / `killall omnitrace-perfetto`
before starting `traced` / `perfetto`. Another approach would be to detect any running instances and skip launching if
it is not necessary.

A running `traced` / `omnitrace-perfetto-traced` can be detected by one of the following ways:

```bash
ss -l | grep -c perfetto # if > 0, traced is running. Exit code of 0 if running, non-zero exit code if not running
ss -l | grep perfetto | wc -l # if > 0, traced is running. Always has exit code of 0
if [ -S /tmp/perfetto-producer -a -S /tmp/perfetto-consumer ]; then echo "traced is running"; fi
```

The `perfetto` / `omnitrace-perfetto` is trickier, in general, using `pgrep` or `ps` can serve as a workaround
or, alternatively, one can always start a `perfetto` / `omnitrace-perfetto` process since, generally, these will
be interrupted (and, thus, will exit) in order to generate the output trace file.

```bash
# if using traced and perfetto
pgrep -a -f perfetto
ps x | grep [p]erfetto
# if using omnitrace-perfetto-traced and omnitrace-perfetto
pgrep -a -f perfetto | grep -v traced
ps x | grep [p]erfetto | grep -v traced
```

#### Perfetto System Backend with External Perfetto Installation

Enable `traced` and `perfetto` in the background:

```bash
if [ $(ss -l | grep -c perfetto | wc -l) -eq 0  ]; then traced --background; fi
perfetto --out ./omnitrace-perfetto.proto --txt -c ${OMNITRACE_ROOT}/share/omnitrace/perfetto.cfg --background
```

Configure omnitrace to use the perfetto system backend via the `--perfetto-backend` option of `omnitrace-run`:

```bash
# enable sampling on the uninstrumented binary
omnitrace-run --sample --trace --perfetto-backend=system -- ./myapp
# trace the instrument the binary
omnitrace-instrument -o ./myapp.inst -- ./myapp
omnitrace-run --trace --perfetto-backend=system -- ./myapp.inst
```

#### Perfetto System Backend with Omnitrace Perfetto Installation

Enable `omnitrace-perfetto-traced` and `omnitrace-perfetto` in the background:

```shell
pkill traced
omnitrace-perfetto-traced --background
omnitrace-perfetto --out ./omnitrace-perfetto.proto --txt -c ${OMNITRACE_ROOT}/share/omnitrace/perfetto.cfg --background
```

Configure omnitrace to use the perfetto system backend via the `--perfetto-backend` option of `omnitrace-run`:

```shell
# enable sampling on the uninstrumented binary
omnitrace-run --sample --trace --perfetto-backend=system -- ./myapp
# trace the instrument the binary
omnitrace-instrument -o ./myapp.inst -- ./myapp
omnitrace-run --trace --perfetto-backend=system -- ./myapp.inst
```

## Timemory Output

Use `omnitrace-avail --components --filename` to view the base filename for each component. E.g.

```shell
$ omnitrace-avail wall_clock -C -f
|---------------------------------|---------------|------------------------|
|            COMPONENT            |   AVAILABLE   |        FILENAME        |
|---------------------------------|---------------|------------------------|
| wall_clock                      |     true      | wall_clock             |
| sampling_wall_clock             |     true      | sampling_wall_clock    |
|---------------------------------|---------------|------------------------|
```

Setting `OMNITRACE_COLLAPSE_THREADS=ON` and/or `OMNITRACE_COLLAPSE_PROCESSES=ON` (only valid with full MPI support) the timemory output
will combine the per-thread and/or per-rank data which have identical call-stacks.

The `OMNITRACE_FLAT_PROFILE` setting will remove all call stack heirarchy. Using `OMNITRACE_FLAT_PROFILE=ON` in combination
with `OMNITRACE_COLLAPSE_THREADS=ON` is a useful configuration for identifying min/max measurements regardless of calling context.
The `OMNITRACE_TIMELINE_PROFILE` setting (with `OMNITRACE_FLAT_PROFILE=OFF`) will effectively generate similar data that can be found
in perfetto. Enabling timeline and flat profiling will effectively generate similar data to `strace`. However, while timemory in general
requires significantly less memory than perfetto, this is not the case in timeline mode so activate this setting with caution.

### Timemory Text Output

> ***Hint: the generation of text output is configurable via `OMNITRACE_TEXT_OUTPUT`***

Timemory text output files are meant for human-consumption (use JSON formats for analysis)
and as such, some fields such as the `LABEL` fields may be truncated for readability.
Modification of the truncation can be changed via the `OMNITRACE_MAX_WIDTH` setting.

#### Timemory Text Output Example

In the below, the `NN` field in `|NN>>>` is the thread ID. If MPI support is enabled, this will be `|MM|NN>>>` and `MM` will be the rank.
If `OMNITRACE_COLLAPSE_THREADS=ON` and `OMNITRACE_COLLAPSE_PROCESSES=ON`, neither the `MM` nor the `NN` will be present unless the
component explicitly sets type-traits which specify that the data is only relevant per-thread or per-process, e.g. the `thread_cpu_clock` clock component.

```console
|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
|                                                                       REAL-CLOCK TIMER (I.E. WALL-CLOCK TIMER)                                                                      |
|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
|                            LABEL                             | COUNT  | DEPTH  |   METRIC   | UNITS  |   SUM     |   MEAN    |   MIN     |   MAX     |   VAR    | STDDEV   | % SELF |
|--------------------------------------------------------------|--------|--------|------------|--------|-----------|-----------|-----------|-----------|----------|----------|--------|
| |00>>> main                                                  |      1 |      0 | wall_clock | sec    | 13.360265 | 13.360265 | 13.360265 | 13.360265 | 0.000000 | 0.000000 |   18.2 |
| |00>>> |_ompt_thread_initial                                 |      1 |      1 | wall_clock | sec    | 10.924161 | 10.924161 | 10.924161 | 10.924161 | 0.000000 | 0.000000 |    0.0 |
| |00>>>   |_ompt_implicit_task                                |      1 |      2 | wall_clock | sec    | 10.923050 | 10.923050 | 10.923050 | 10.923050 | 0.000000 | 0.000000 |    0.1 |
| |00>>>     |_ompt_parallel [parallelism=12]                  |      1 |      3 | wall_clock | sec    | 10.915026 | 10.915026 | 10.915026 | 10.915026 | 0.000000 | 0.000000 |    0.0 |
| |00>>>       |_ompt_implicit_task                            |      1 |      4 | wall_clock | sec    | 10.647951 | 10.647951 | 10.647951 | 10.647951 | 0.000000 | 0.000000 |    0.0 |
| |00>>>         |_ompt_work_loop                              |    156 |      5 | wall_clock | sec    |  0.000812 |  0.000005 |  0.000001 |  0.000212 | 0.000000 | 0.000018 |  100.0 |
| |00>>>         |_ompt_work_single_executor                   |     40 |      5 | wall_clock | sec    |  0.000016 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |00>>>         |_ompt_sync_region_barrier_implicit           |    308 |      5 | wall_clock | sec    |  0.000629 |  0.000002 |  0.000001 |  0.000017 | 0.000000 | 0.000002 |  100.0 |
| |00>>>         |_conj_grad                                   |     76 |      5 | wall_clock | sec    | 10.641165 |  0.140015 |  0.131894 |  0.155099 | 0.000017 | 0.004080 |    1.0 |
| |00>>>           |_ompt_work_single_executor                 |    803 |      6 | wall_clock | sec    |  0.000292 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |00>>>           |_ompt_work_loop                            |   7904 |      6 | wall_clock | sec    |  7.420265 |  0.000939 |  0.000005 |  0.006974 | 0.000003 | 0.001613 |  100.0 |
| |00>>>           |_ompt_sync_region_barrier_implicit         |   6004 |      6 | wall_clock | sec    |  0.283160 |  0.000047 |  0.000001 |  0.004087 | 0.000000 | 0.000303 |  100.0 |
| |00>>>           |_ompt_sync_region_barrier_implementation   |   3952 |      6 | wall_clock | sec    |  2.829252 |  0.000716 |  0.000007 |  0.009005 | 0.000001 | 0.000985 |   99.7 |
| |00>>>             |_ompt_sync_region_reduction              |  15808 |      7 | wall_clock | sec    |  0.009142 |  0.000001 |  0.000000 |  0.000007 | 0.000000 | 0.000000 |  100.0 |
| |00>>>           |_ompt_work_single_other                    |   1249 |      6 | wall_clock | sec    |  0.000270 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |00>>>         |_ompt_work_single_other                      |    114 |      5 | wall_clock | sec    |  0.000024 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |00>>>         |_ompt_sync_region_barrier_implementation     |     76 |      5 | wall_clock | sec    |  0.000876 |  0.000012 |  0.000008 |  0.000025 | 0.000000 | 0.000003 |   84.4 |
| |00>>>           |_ompt_sync_region_reduction                |    304 |      6 | wall_clock | sec    |  0.000136 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |00>>>         |_ompt_master                                 |    226 |      5 | wall_clock | sec    |  0.001978 |  0.000009 |  0.000000 |  0.000038 | 0.000000 | 0.000012 |  100.0 |
| |11>>>       |_ompt_thread_worker                            |      1 |      4 | wall_clock | sec    | 10.656145 | 10.656145 | 10.656145 | 10.656145 | 0.000000 | 0.000000 |    0.1 |
| |11>>>         |_ompt_implicit_task                          |      1 |      5 | wall_clock | sec    | 10.649183 | 10.649183 | 10.649183 | 10.649183 | 0.000000 | 0.000000 |    0.0 |
| |11>>>           |_ompt_work_loop                            |    156 |      6 | wall_clock | sec    |  0.000852 |  0.000005 |  0.000002 |  0.000230 | 0.000000 | 0.000019 |  100.0 |
| |11>>>           |_ompt_work_single_other                    |    149 |      6 | wall_clock | sec    |  0.000035 |  0.000000 |  0.000000 |  0.000000 | 0.000000 | 0.000000 |  100.0 |
| |11>>>           |_ompt_sync_region_barrier_implicit         |    308 |      6 | wall_clock | sec    |  0.004135 |  0.000013 |  0.000001 |  0.001233 | 0.000000 | 0.000070 |  100.0 |
| |11>>>           |_conj_grad                                 |     76 |      6 | wall_clock | sec    | 10.641302 |  0.140017 |  0.131896 |  0.155102 | 0.000017 | 0.004080 |    0.6 |
| |11>>>             |_ompt_work_single_other                  |   2023 |      7 | wall_clock | sec    |  0.000458 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |11>>>             |_ompt_work_loop                          |   7904 |      7 | wall_clock | sec    |  8.253555 |  0.001044 |  0.000005 |  0.008021 | 0.000003 | 0.001790 |  100.0 |
| |11>>>             |_ompt_sync_region_barrier_implicit       |   6004 |      7 | wall_clock | sec    |  0.263840 |  0.000044 |  0.000001 |  0.004087 | 0.000000 | 0.000297 |  100.0 |
| |11>>>             |_ompt_sync_region_barrier_implementation |   3952 |      7 | wall_clock | sec    |  2.059823 |  0.000521 |  0.000007 |  0.009508 | 0.000001 | 0.000863 |  100.0 |
| |11>>>             |_ompt_work_single_executor               |     29 |      7 | wall_clock | sec    |  0.000011 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |11>>>           |_ompt_work_single_executor                 |      5 |      6 | wall_clock | sec    |  0.000002 |  0.000000 |  0.000000 |  0.000000 | 0.000000 | 0.000000 |  100.0 |
| |11>>>           |_ompt_sync_region_barrier_implementation   |     76 |      6 | wall_clock | sec    |  0.000975 |  0.000013 |  0.000008 |  0.000024 | 0.000000 | 0.000003 |  100.0 |
| |10>>>       |_ompt_thread_worker                            |      1 |      4 | wall_clock | sec    | 10.681664 | 10.681664 | 10.681664 | 10.681664 | 0.000000 | 0.000000 |    0.3 |
| |10>>>         |_ompt_implicit_task                          |      1 |      5 | wall_clock | sec    | 10.649158 | 10.649158 | 10.649158 | 10.649158 | 0.000000 | 0.000000 |    0.0 |
| |10>>>           |_ompt_work_loop                            |    156 |      6 | wall_clock | sec    |  0.000863 |  0.000006 |  0.000002 |  0.000231 | 0.000000 | 0.000019 |  100.0 |
| |10>>>           |_ompt_work_single_other                    |    140 |      6 | wall_clock | sec    |  0.000037 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |10>>>           |_ompt_sync_region_barrier_implicit         |    308 |      6 | wall_clock | sec    |  0.004149 |  0.000013 |  0.000001 |  0.001221 | 0.000000 | 0.000070 |  100.0 |
| |10>>>           |_conj_grad                                 |     76 |      6 | wall_clock | sec    | 10.641288 |  0.140017 |  0.131896 |  0.155101 | 0.000017 | 0.004080 |    0.7 |
| |10>>>             |_ompt_work_single_other                  |   1883 |      7 | wall_clock | sec    |  0.000487 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |10>>>             |_ompt_work_loop                          |   7904 |      7 | wall_clock | sec    |  8.174545 |  0.001034 |  0.000005 |  0.006899 | 0.000003 | 0.001766 |  100.0 |
| |10>>>             |_ompt_sync_region_barrier_implicit       |   6004 |      7 | wall_clock | sec    |  0.268808 |  0.000045 |  0.000001 |  0.004087 | 0.000000 | 0.000299 |  100.0 |
| |10>>>             |_ompt_sync_region_barrier_implementation |   3952 |      7 | wall_clock | sec    |  2.126988 |  0.000538 |  0.000007 |  0.009843 | 0.000001 | 0.000872 |   99.9 |
| |10>>>               |_ompt_sync_region_reduction            |   3952 |      8 | wall_clock | sec    |  0.002574 |  0.000001 |  0.000000 |  0.000014 | 0.000000 | 0.000000 |  100.0 |
| |10>>>             |_ompt_work_single_executor               |    169 |      7 | wall_clock | sec    |  0.000072 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |10>>>           |_ompt_sync_region_barrier_implementation   |     76 |      6 | wall_clock | sec    |  0.000954 |  0.000013 |  0.000009 |  0.000023 | 0.000000 | 0.000003 |   95.9 |
| |10>>>             |_ompt_sync_region_reduction              |     76 |      7 | wall_clock | sec    |  0.000039 |  0.000001 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |10>>>           |_ompt_work_single_executor                 |     14 |      6 | wall_clock | sec    |  0.000006 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |09>>>       |_ompt_thread_worker                            |      1 |      4 | wall_clock | sec    | 10.686552 | 10.686552 | 10.686552 | 10.686552 | 0.000000 | 0.000000 |    0.3 |
| |09>>>         |_ompt_implicit_task                          |      1 |      5 | wall_clock | sec    | 10.649151 | 10.649151 | 10.649151 | 10.649151 | 0.000000 | 0.000000 |    0.0 |
| |09>>>           |_ompt_work_loop                            |    156 |      6 | wall_clock | sec    |  0.000880 |  0.000006 |  0.000002 |  0.000258 | 0.000000 | 0.000021 |  100.0 |
| |09>>>           |_ompt_work_single_other                    |    148 |      6 | wall_clock | sec    |  0.000034 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |09>>>           |_ompt_sync_region_barrier_implicit         |    308 |      6 | wall_clock | sec    |  0.004129 |  0.000013 |  0.000001 |  0.001210 | 0.000000 | 0.000069 |  100.0 |
| |09>>>           |_conj_grad                                 |     76 |      6 | wall_clock | sec    | 10.641308 |  0.140017 |  0.131895 |  0.155102 | 0.000017 | 0.004080 |    0.7 |
| |09>>>             |_ompt_work_single_other                  |   2043 |      7 | wall_clock | sec    |  0.000473 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |09>>>             |_ompt_work_loop                          |   7904 |      7 | wall_clock | sec    |  7.977001 |  0.001009 |  0.000005 |  0.007325 | 0.000003 | 0.001732 |  100.0 |
| |09>>>             |_ompt_sync_region_barrier_implicit       |   6004 |      7 | wall_clock | sec    |  0.242996 |  0.000040 |  0.000001 |  0.004087 | 0.000000 | 0.000284 |  100.0 |
| |09>>>             |_ompt_sync_region_barrier_implementation |   3952 |      7 | wall_clock | sec    |  2.350895 |  0.000595 |  0.000007 |  0.008689 | 0.000001 | 0.000926 |  100.0 |
| |09>>>             |_ompt_work_single_executor               |      9 |      7 | wall_clock | sec    |  0.000004 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |09>>>           |_ompt_sync_region_barrier_implementation   |     76 |      6 | wall_clock | sec    |  0.000973 |  0.000013 |  0.000008 |  0.000025 | 0.000000 | 0.000003 |  100.0 |
| |09>>>           |_ompt_work_single_executor                 |      6 |      6 | wall_clock | sec    |  0.000002 |  0.000000 |  0.000000 |  0.000000 | 0.000000 | 0.000000 |  100.0 |
| |08>>>       |_ompt_thread_worker                            |      1 |      4 | wall_clock | sec    | 10.721622 | 10.721622 | 10.721622 | 10.721622 | 0.000000 | 0.000000 |    0.7 |
| |08>>>         |_ompt_implicit_task                          |      1 |      5 | wall_clock | sec    | 10.649135 | 10.649135 | 10.649135 | 10.649135 | 0.000000 | 0.000000 |    0.0 |
| |08>>>           |_ompt_work_loop                            |    156 |      6 | wall_clock | sec    |  0.000839 |  0.000005 |  0.000001 |  0.000231 | 0.000000 | 0.000019 |  100.0 |
| |08>>>           |_ompt_work_single_other                    |    141 |      6 | wall_clock | sec    |  0.000030 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |08>>>           |_ompt_sync_region_barrier_implicit         |    308 |      6 | wall_clock | sec    |  0.004114 |  0.000013 |  0.000001 |  0.001198 | 0.000000 | 0.000069 |  100.0 |
| |08>>>           |_conj_grad                                 |     76 |      6 | wall_clock | sec    | 10.641294 |  0.140017 |  0.131896 |  0.155101 | 0.000017 | 0.004080 |    0.6 |
| |08>>>             |_ompt_work_single_other                  |   1742 |      7 | wall_clock | sec    |  0.000392 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |08>>>             |_ompt_work_loop                          |   7904 |      7 | wall_clock | sec    |  8.306388 |  0.001051 |  0.000005 |  0.007886 | 0.000003 | 0.001795 |  100.0 |
| |08>>>             |_ompt_sync_region_barrier_implicit       |   6004 |      7 | wall_clock | sec    |  0.274358 |  0.000046 |  0.000001 |  0.004090 | 0.000000 | 0.000302 |  100.0 |
| |08>>>             |_ompt_sync_region_barrier_implementation |   3952 |      7 | wall_clock | sec    |  1.991251 |  0.000504 |  0.000007 |  0.008694 | 0.000001 | 0.000844 |   99.8 |
| |08>>>               |_ompt_sync_region_reduction            |   7904 |      8 | wall_clock | sec    |  0.003816 |  0.000000 |  0.000000 |  0.000017 | 0.000000 | 0.000000 |  100.0 |
| |08>>>             |_ompt_work_single_executor               |    310 |      7 | wall_clock | sec    |  0.000112 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |08>>>           |_ompt_sync_region_barrier_implementation   |     76 |      6 | wall_clock | sec    |  0.000955 |  0.000013 |  0.000009 |  0.000026 | 0.000000 | 0.000003 |   93.7 |
| |08>>>             |_ompt_sync_region_reduction              |    152 |      7 | wall_clock | sec    |  0.000060 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |08>>>           |_ompt_work_single_executor                 |     13 |      6 | wall_clock | sec    |  0.000005 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |07>>>       |_ompt_thread_worker                            |      1 |      4 | wall_clock | sec    | 10.747282 | 10.747282 | 10.747282 | 10.747282 | 0.000000 | 0.000000 |    0.9 |
| |07>>>         |_ompt_implicit_task                          |      1 |      5 | wall_clock | sec    | 10.649093 | 10.649093 | 10.649093 | 10.649093 | 0.000000 | 0.000000 |    0.0 |
| |07>>>           |_ompt_work_loop                            |    156 |      6 | wall_clock | sec    |  0.000923 |  0.000006 |  0.000002 |  0.000231 | 0.000000 | 0.000019 |  100.0 |
| |07>>>           |_ompt_work_single_other                    |    152 |      6 | wall_clock | sec    |  0.000048 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |07>>>           |_ompt_sync_region_barrier_implicit         |    308 |      6 | wall_clock | sec    |  0.003981 |  0.000013 |  0.000001 |  0.001186 | 0.000000 | 0.000068 |  100.0 |
| |07>>>           |_conj_grad                                 |     76 |      6 | wall_clock | sec    | 10.641295 |  0.140017 |  0.131896 |  0.155101 | 0.000017 | 0.004080 |    0.7 |
| |07>>>             |_ompt_work_single_other                  |   2043 |      7 | wall_clock | sec    |  0.000648 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |07>>>             |_ompt_work_loop                          |   7904 |      7 | wall_clock | sec    |  7.978811 |  0.001009 |  0.000005 |  0.006728 | 0.000003 | 0.001732 |  100.0 |
| |07>>>             |_ompt_sync_region_barrier_implicit       |   6004 |      7 | wall_clock | sec    |  0.199939 |  0.000033 |  0.000001 |  0.004086 | 0.000000 | 0.000255 |  100.0 |
| |07>>>             |_ompt_sync_region_barrier_implementation |   3952 |      7 | wall_clock | sec    |  2.385843 |  0.000604 |  0.000009 |  0.009039 | 0.000001 | 0.000938 |  100.0 |
| |07>>>             |_ompt_work_single_executor               |      9 |      7 | wall_clock | sec    |  0.000004 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |07>>>           |_ompt_sync_region_barrier_implementation   |     76 |      6 | wall_clock | sec    |  0.000905 |  0.000012 |  0.000010 |  0.000025 | 0.000000 | 0.000003 |  100.0 |
| |07>>>           |_ompt_work_single_executor                 |      2 |      6 | wall_clock | sec    |  0.000001 |  0.000001 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |06>>>       |_ompt_thread_worker                            |      1 |      4 | wall_clock | sec    | 10.772278 | 10.772278 | 10.772278 | 10.772278 | 0.000000 | 0.000000 |    1.1 |
| |06>>>         |_ompt_implicit_task                          |      1 |      5 | wall_clock | sec    | 10.649092 | 10.649092 | 10.649092 | 10.649092 | 0.000000 | 0.000000 |    0.0 |
| |06>>>           |_ompt_work_loop                            |    156 |      6 | wall_clock | sec    |  0.000888 |  0.000006 |  0.000002 |  0.000236 | 0.000000 | 0.000020 |  100.0 |
| |06>>>           |_ompt_work_single_other                    |    153 |      6 | wall_clock | sec    |  0.000037 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |06>>>           |_ompt_sync_region_barrier_implicit         |    308 |      6 | wall_clock | sec    |  0.004090 |  0.000013 |  0.000001 |  0.001175 | 0.000000 | 0.000067 |  100.0 |
| |06>>>           |_conj_grad                                 |     76 |      6 | wall_clock | sec    | 10.641317 |  0.140017 |  0.131896 |  0.155101 | 0.000017 | 0.004080 |    0.8 |
| |06>>>             |_ompt_work_single_other                  |   2041 |      7 | wall_clock | sec    |  0.000476 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |06>>>             |_ompt_work_loop                          |   7904 |      7 | wall_clock | sec    |  7.467961 |  0.000945 |  0.000005 |  0.010712 | 0.000003 | 0.001627 |  100.0 |
| |06>>>             |_ompt_sync_region_barrier_implicit       |   6004 |      7 | wall_clock | sec    |  0.250883 |  0.000042 |  0.000001 |  0.004087 | 0.000000 | 0.000285 |  100.0 |
| |06>>>             |_ompt_sync_region_barrier_implementation |   3952 |      7 | wall_clock | sec    |  2.838733 |  0.000718 |  0.000009 |  0.009015 | 0.000001 | 0.001015 |   99.9 |
| |06>>>               |_ompt_sync_region_reduction            |   3952 |      8 | wall_clock | sec    |  0.003334 |  0.000001 |  0.000000 |  0.000025 | 0.000000 | 0.000001 |  100.0 |
| |06>>>             |_ompt_work_single_executor               |     11 |      7 | wall_clock | sec    |  0.000005 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |06>>>           |_ompt_sync_region_barrier_implementation   |     76 |      6 | wall_clock | sec    |  0.000940 |  0.000012 |  0.000009 |  0.000025 | 0.000000 | 0.000003 |   95.4 |
| |06>>>             |_ompt_sync_region_reduction              |     76 |      7 | wall_clock | sec    |  0.000044 |  0.000001 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |06>>>           |_ompt_work_single_executor                 |      1 |      6 | wall_clock | sec    |  0.000000 |  0.000000 |  0.000000 |  0.000000 | 0.000000 | 0.000000 |  100.0 |
| |05>>>       |_ompt_thread_worker                            |      1 |      4 | wall_clock | sec    | 10.797950 | 10.797950 | 10.797950 | 10.797950 | 0.000000 | 0.000000 |    1.4 |
| |05>>>         |_ompt_implicit_task                          |      1 |      5 | wall_clock | sec    | 10.649072 | 10.649072 | 10.649072 | 10.649072 | 0.000000 | 0.000000 |    0.0 |
| |05>>>           |_ompt_work_loop                            |    156 |      6 | wall_clock | sec    |  0.000879 |  0.000006 |  0.000001 |  0.000248 | 0.000000 | 0.000021 |  100.0 |
| |05>>>           |_ompt_work_single_other                    |    142 |      6 | wall_clock | sec    |  0.000034 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |05>>>           |_ompt_sync_region_barrier_implicit         |    308 |      6 | wall_clock | sec    |  0.004062 |  0.000013 |  0.000002 |  0.001163 | 0.000000 | 0.000067 |  100.0 |
| |05>>>           |_conj_grad                                 |     76 |      6 | wall_clock | sec    | 10.641291 |  0.140017 |  0.131896 |  0.155101 | 0.000017 | 0.004080 |    0.7 |
| |05>>>             |_ompt_work_single_other                  |   2038 |      7 | wall_clock | sec    |  0.000500 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |05>>>             |_ompt_work_loop                          |   7904 |      7 | wall_clock | sec    |  8.279191 |  0.001047 |  0.000005 |  0.006596 | 0.000003 | 0.001792 |  100.0 |
| |05>>>             |_ompt_sync_region_barrier_implicit       |   6004 |      7 | wall_clock | sec    |  0.250939 |  0.000042 |  0.000001 |  0.004090 | 0.000000 | 0.000286 |  100.0 |
| |05>>>             |_ompt_sync_region_barrier_implementation |   3952 |      7 | wall_clock | sec    |  2.039013 |  0.000516 |  0.000009 |  0.008689 | 0.000001 | 0.000855 |  100.0 |
| |05>>>             |_ompt_work_single_executor               |     14 |      7 | wall_clock | sec    |  0.000005 |  0.000000 |  0.000000 |  0.000000 | 0.000000 | 0.000000 |  100.0 |
| |05>>>           |_ompt_sync_region_barrier_implementation   |     76 |      6 | wall_clock | sec    |  0.000926 |  0.000012 |  0.000009 |  0.000023 | 0.000000 | 0.000003 |  100.0 |
| |05>>>           |_ompt_work_single_executor                 |     12 |      6 | wall_clock | sec    |  0.000005 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |04>>>       |_ompt_thread_worker                            |      1 |      4 | wall_clock | sec    | 10.825935 | 10.825935 | 10.825935 | 10.825935 | 0.000000 | 0.000000 |    1.6 |
| |04>>>         |_ompt_implicit_task                          |      1 |      5 | wall_clock | sec    | 10.649068 | 10.649068 | 10.649068 | 10.649068 | 0.000000 | 0.000000 |    0.0 |
| |04>>>           |_ompt_work_loop                            |    156 |      6 | wall_clock | sec    |  0.000884 |  0.000006 |  0.000002 |  0.000245 | 0.000000 | 0.000020 |  100.0 |
| |04>>>           |_ompt_work_single_other                    |    150 |      6 | wall_clock | sec    |  0.000034 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |04>>>           |_ompt_sync_region_barrier_implicit         |    308 |      6 | wall_clock | sec    |  0.004069 |  0.000013 |  0.000001 |  0.001151 | 0.000000 | 0.000066 |  100.0 |
| |04>>>           |_conj_grad                                 |     76 |      6 | wall_clock | sec    | 10.641300 |  0.140017 |  0.131896 |  0.155101 | 0.000017 | 0.004080 |    1.1 |
| |04>>>             |_ompt_work_single_other                  |   2041 |      7 | wall_clock | sec    |  0.000448 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |04>>>             |_ompt_work_loop                          |   7904 |      7 | wall_clock | sec    |  7.438393 |  0.000941 |  0.000005 |  0.007090 | 0.000003 | 0.001624 |  100.0 |
| |04>>>             |_ompt_sync_region_barrier_implicit       |   6004 |      7 | wall_clock | sec    |  0.270654 |  0.000045 |  0.000001 |  0.004090 | 0.000000 | 0.000295 |  100.0 |
| |04>>>             |_ompt_sync_region_barrier_implementation |   3952 |      7 | wall_clock | sec    |  2.819165 |  0.000713 |  0.000009 |  0.008379 | 0.000001 | 0.001013 |   99.9 |
| |04>>>               |_ompt_sync_region_reduction            |   7904 |      8 | wall_clock | sec    |  0.003932 |  0.000000 |  0.000000 |  0.000015 | 0.000000 | 0.000000 |  100.0 |
| |04>>>             |_ompt_work_single_executor               |     11 |      7 | wall_clock | sec    |  0.000005 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |04>>>           |_ompt_sync_region_barrier_implementation   |     76 |      6 | wall_clock | sec    |  0.000936 |  0.000012 |  0.000009 |  0.000025 | 0.000000 | 0.000003 |   93.2 |
| |04>>>             |_ompt_sync_region_reduction              |    152 |      7 | wall_clock | sec    |  0.000064 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |04>>>           |_ompt_work_single_executor                 |      4 |      6 | wall_clock | sec    |  0.000001 |  0.000000 |  0.000000 |  0.000000 | 0.000000 | 0.000000 |  100.0 |
| |03>>>       |_ompt_thread_worker                            |      1 |      4 | wall_clock | sec    | 10.849322 | 10.849322 | 10.849322 | 10.849322 | 0.000000 | 0.000000 |    1.8 |
| |03>>>         |_ompt_implicit_task                          |      1 |      5 | wall_clock | sec    | 10.649075 | 10.649075 | 10.649075 | 10.649075 | 0.000000 | 0.000000 |    0.0 |
| |03>>>           |_ompt_work_loop                            |    156 |      6 | wall_clock | sec    |  0.000861 |  0.000006 |  0.000002 |  0.000238 | 0.000000 | 0.000020 |  100.0 |
| |03>>>           |_ompt_work_single_other                    |    120 |      6 | wall_clock | sec    |  0.000028 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |03>>>           |_ompt_sync_region_barrier_implicit         |    308 |      6 | wall_clock | sec    |  0.003993 |  0.000013 |  0.000001 |  0.001138 | 0.000000 | 0.000065 |  100.0 |
| |03>>>           |_conj_grad                                 |     76 |      6 | wall_clock | sec    | 10.641302 |  0.140017 |  0.131896 |  0.155101 | 0.000017 | 0.004080 |    0.8 |
| |03>>>             |_ompt_work_single_other                  |   1756 |      7 | wall_clock | sec    |  0.000426 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |03>>>             |_ompt_work_loop                          |   7904 |      7 | wall_clock | sec    |  8.005617 |  0.001013 |  0.000005 |  0.011500 | 0.000003 | 0.001741 |  100.0 |
| |03>>>             |_ompt_sync_region_barrier_implicit       |   6004 |      7 | wall_clock | sec    |  0.231485 |  0.000039 |  0.000001 |  0.004086 | 0.000000 | 0.000277 |  100.0 |
| |03>>>             |_ompt_sync_region_barrier_implementation |   3952 |      7 | wall_clock | sec    |  2.320428 |  0.000587 |  0.000009 |  0.010868 | 0.000001 | 0.000912 |  100.0 |
| |03>>>             |_ompt_work_single_executor               |    296 |      7 | wall_clock | sec    |  0.000120 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |03>>>           |_ompt_sync_region_barrier_implementation   |     76 |      6 | wall_clock | sec    |  0.000967 |  0.000013 |  0.000010 |  0.000023 | 0.000000 | 0.000003 |  100.0 |
| |03>>>           |_ompt_work_single_executor                 |     34 |      6 | wall_clock | sec    |  0.000013 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |02>>>       |_ompt_thread_worker                            |      1 |      4 | wall_clock | sec    | 10.876387 | 10.876387 | 10.876387 | 10.876387 | 0.000000 | 0.000000 |    2.1 |
| |02>>>         |_ompt_implicit_task                          |      1 |      5 | wall_clock | sec    | 10.649050 | 10.649050 | 10.649050 | 10.649050 | 0.000000 | 0.000000 |    0.0 |
| |02>>>           |_ompt_work_loop                            |    156 |      6 | wall_clock | sec    |  0.000924 |  0.000006 |  0.000001 |  0.000241 | 0.000000 | 0.000020 |  100.0 |
| |02>>>           |_ompt_work_single_other                    |    139 |      6 | wall_clock | sec    |  0.000040 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |02>>>           |_ompt_sync_region_barrier_implicit         |    308 |      6 | wall_clock | sec    |  0.003972 |  0.000013 |  0.000001 |  0.001127 | 0.000000 | 0.000064 |  100.0 |
| |02>>>           |_conj_grad                                 |     76 |      6 | wall_clock | sec    | 10.641287 |  0.140017 |  0.131895 |  0.155101 | 0.000017 | 0.004080 |    0.7 |
| |02>>>             |_ompt_work_single_other                  |   1902 |      7 | wall_clock | sec    |  0.000553 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |02>>>             |_ompt_work_loop                          |   7904 |      7 | wall_clock | sec    |  7.906688 |  0.001000 |  0.000005 |  0.007068 | 0.000003 | 0.001713 |  100.0 |
| |02>>>             |_ompt_sync_region_barrier_implicit       |   6004 |      7 | wall_clock | sec    |  0.261367 |  0.000044 |  0.000001 |  0.004088 | 0.000000 | 0.000295 |  100.0 |
| |02>>>             |_ompt_sync_region_barrier_implementation |   3952 |      7 | wall_clock | sec    |  2.402362 |  0.000608 |  0.000009 |  0.010399 | 0.000001 | 0.000944 |   99.9 |
| |02>>>               |_ompt_sync_region_reduction            |   3952 |      8 | wall_clock | sec    |  0.002937 |  0.000001 |  0.000000 |  0.000021 | 0.000000 | 0.000000 |  100.0 |
| |02>>>             |_ompt_work_single_executor               |    150 |      7 | wall_clock | sec    |  0.000073 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |02>>>           |_ompt_sync_region_barrier_implementation   |     76 |      6 | wall_clock | sec    |  0.000895 |  0.000012 |  0.000009 |  0.000026 | 0.000000 | 0.000003 |   95.2 |
| |02>>>             |_ompt_sync_region_reduction              |     76 |      7 | wall_clock | sec    |  0.000043 |  0.000001 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |02>>>           |_ompt_work_single_executor                 |     15 |      6 | wall_clock | sec    |  0.000007 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |01>>>       |_ompt_thread_worker                            |      1 |      4 | wall_clock | sec    | 10.901650 | 10.901650 | 10.901650 | 10.901650 | 0.000000 | 0.000000 |    2.3 |
| |01>>>         |_ompt_implicit_task                          |      1 |      5 | wall_clock | sec    | 10.649017 | 10.649017 | 10.649017 | 10.649017 | 0.000000 | 0.000000 |    0.0 |
| |01>>>           |_ompt_work_loop                            |    156 |      6 | wall_clock | sec    |  0.000863 |  0.000006 |  0.000001 |  0.000231 | 0.000000 | 0.000019 |  100.0 |
| |01>>>           |_ompt_work_single_other                    |    146 |      6 | wall_clock | sec    |  0.000033 |  0.000000 |  0.000000 |  0.000000 | 0.000000 | 0.000000 |  100.0 |
| |01>>>           |_ompt_sync_region_barrier_implicit         |    308 |      6 | wall_clock | sec    |  0.004012 |  0.000013 |  0.000001 |  0.001115 | 0.000000 | 0.000064 |  100.0 |
| |01>>>           |_conj_grad                                 |     76 |      6 | wall_clock | sec    | 10.641316 |  0.140017 |  0.131895 |  0.155101 | 0.000017 | 0.004080 |    0.8 |
| |01>>>             |_ompt_work_single_other                  |   1811 |      7 | wall_clock | sec    |  0.000403 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |01>>>             |_ompt_work_loop                          |   7904 |      7 | wall_clock | sec    |  7.410337 |  0.000938 |  0.000005 |  0.010556 | 0.000003 | 0.001610 |  100.0 |
| |01>>>             |_ompt_sync_region_barrier_implicit       |   6004 |      7 | wall_clock | sec    |  0.202494 |  0.000034 |  0.000001 |  0.003521 | 0.000000 | 0.000256 |  100.0 |
| |01>>>             |_ompt_sync_region_barrier_implementation |   3952 |      7 | wall_clock | sec    |  2.943604 |  0.000745 |  0.000008 |  0.009033 | 0.000001 | 0.001024 |  100.0 |
| |01>>>             |_ompt_work_single_executor               |    241 |      7 | wall_clock | sec    |  0.000093 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |01>>>           |_ompt_sync_region_barrier_implementation   |     76 |      6 | wall_clock | sec    |  0.000917 |  0.000012 |  0.000009 |  0.000026 | 0.000000 | 0.000003 |  100.0 |
| |01>>>           |_ompt_work_single_executor                 |      8 |      6 | wall_clock | sec    |  0.000004 |  0.000000 |  0.000000 |  0.000001 | 0.000000 | 0.000000 |  100.0 |
| |00>>>   |_c_print_results                                   |      1 |      2 | wall_clock | sec    |  0.000049 |  0.000049 |  0.000049 |  0.000049 | 0.000000 | 0.000000 |  100.0 |
|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
```

### Timemory JSON Output

> ***Hint: the generation of flat JSON output is configurable via `OMNITRACE_JSON_OUTPUT`.***
> ***The generation of hierarchical JSON data is configurable via `OMNITRACE_TREE_OUTPUT`.***

Timemory represents the data within the JSON output in two forms: a flat structure and a hierarchical structure.
The flat JSON data represents the data similar to the text files: the hierarchical information
is represented by the indentation of the `"prefix"` field and the `"depth"` field.
The hierarchical JSON contains additional information with respect to inclusive and exclusive value, however,
it's structure requires processing through recursion. This section of the JSON supports analysis
by [hatchet](https://github.com/hatchet/hatchet).
All the data entries for the flat structure are in a single JSON array.
This format is easier than the hierarchical format to write a simple Python script for post-processing.

#### Timemory JSON Output Sample

In the JSON below, the flat data starts at `["timemory"]["wall_clock"]["ranks"]`
and the hierarchical data starts at `["timemory"]["wall_clock"]["graph"]`.
E.g., accessing the name (prefix) of the nth entry in the flat data layout is:
`["timemory"]["wall_clock"]["ranks"][0]["graph"][<N>]["prefix"]`. When full MPI
support is enable, the per-rank data in flat layout will be represented
in as an entry in the "ranks" array; in the hierarchical data structure,
the per-rank data is represented as entry in the "mpi" array (but "graph"
is used in lieu of "mpi" when full MPI support is enabled).
In the hierarchical layout, all data for the process is all a child of a (dummy)
root node (which has the name `unknown-hash=0`).

```json
{
    "timemory": {
        "wall_clock": {
            "properties": {
                "cereal_class_version": 0,
                "value": 78,
                "enum": "WALL_CLOCK",
                "id": "wall_clock",
                "ids": [
                    "real_clock",
                    "virtual_clock",
                    "wall_clock"
                ]
            },
            "type": "wall_clock",
            "description": "Real-clock timer (i.e. wall-clock timer)",
            "unit_value": 1000000000,
            "unit_repr": "sec",
            "thread_scope_only": false,
            "thread_count": 2,
            "mpi_size": 1,
            "upcxx_size": 1,
            "process_count": 1,
            "num_ranks": 1,
            "concurrency": 2,
            "ranks": [
                {
                    "rank": 0,
                    "graph_size": 112,
                    "graph": [
                        {
                            "hash": 17481650134347108265,
                            "prefix": "|0>>> main",
                            "depth": 0,
                            "entry": {
                                "cereal_class_version": 0,
                                "laps": 1,
                                "value": 894743517,
                                "accum": 894743517,
                                "repr_data": 0.894743517,
                                "repr_display": 0.894743517
                            },
                            "stats": {
                                "cereal_class_version": 0,
                                "sum": 0.894743517,
                                "count": 1,
                                "min": 0.894743517,
                                "max": 0.894743517,
                                "sqr": 0.8005659612135293,
                                "mean": 0.894743517,
                                "stddev": 0.0
                            },
                            "rolling_hash": 17481650134347108265
                        },
                        {
                            "hash": 3455444288293231339,
                            "prefix": "|0>>> |_read_input",
                            "depth": 1,
                            "entry": {
                                "laps": 1,
                                "value": 9808,
                                "accum": 9808,
                                "repr_data": 9.808e-06,
                                "repr_display": 9.808e-06
                            },
                            "stats": {
                                "sum": 9.808e-06,
                                "count": 1,
                                "min": 9.808e-06,
                                "max": 9.808e-06,
                                "sqr": 9.6196864e-11,
                                "mean": 9.808e-06,
                                "stddev": 0.0
                            },
                            "rolling_hash": 2490350348930787988
                        },
                        {
                            "hash": 8456966793631718807,
                            "prefix": "|0>>> |_setcoeff",
                            "depth": 1,
                            "entry": {
                                "laps": 1,
                                "value": 922,
                                "accum": 922,
                                "repr_data": 9.22e-07,
                                "repr_display": 9.22e-07
                            },
                            "stats": {
                                "sum": 9.22e-07,
                                "count": 1,
                                "min": 9.22e-07,
                                "max": 9.22e-07,
                                "sqr": 8.50084e-13,
                                "mean": 9.22e-07,
                                "stddev": 0.0
                            },
                            "rolling_hash": 7491872854269275456
                        },
                        {
                            "hash": 6107876127803219007,
                            "prefix": "|0>>> |_ompt_thread_initial",
                            "depth": 1,
                            "entry": {
                                "laps": 1,
                                "value": 896506392,
                                "accum": 896506392,
                                "repr_data": 0.896506392,
                                "repr_display": 0.896506392
                            },
                            "stats": {
                                "sum": 0.896506392,
                                "count": 1,
                                "min": 0.896506392,
                                "max": 0.896506392,
                                "sqr": 0.8037237108968578,
                                "mean": 0.896506392,
                                "stddev": 0.0
                            },
                            "rolling_hash": 5142782188440775656
                        },
                        {
                            "hash": 15402802091993617561,
                            "prefix": "|0>>>   |_ompt_implicit_task",
                            "depth": 2,
                            "entry": {
                                "laps": 1,
                                "value": 896479111,
                                "accum": 896479111,
                                "repr_data": 0.896479111,
                                "repr_display": 0.896479111
                            },
                            "stats": {
                                "sum": 0.896479111,
                                "count": 1,
                                "min": 0.896479111,
                                "max": 0.896479111,
                                "sqr": 0.8036747964593504,
                                "mean": 0.896479111,
                                "stddev": 0.0
                            },
                            "rolling_hash": 2098840206724841601                        },
                        {
                            "..." : "... etc. ..."
                        }
                    ]
                }
            ],
            "graph": [
                [
                    {
                        "cereal_class_version": 0,
                        "node": {
                            "hash": 0,
                            "prefix": "unknown-hash=0",
                            "tid": [
                                0
                            ],
                            "pid": [
                                2539175
                            ],
                            "depth": 0,
                            "is_dummy": false,
                            "inclusive": {
                                "entry": {
                                    "laps": 0,
                                    "value": 0,
                                    "accum": 0,
                                    "repr_data": 0.0,
                                    "repr_display": 0.0
                                },
                                "stats": {
                                    "sum": 0.0,
                                    "count": 0,
                                    "min": 0.0,
                                    "max": 0.0,
                                    "sqr": 0.0,
                                    "mean": 0.0,
                                    "stddev": 0.0
                                }
                            },
                            "exclusive": {
                                "entry": {
                                    "laps": 0,
                                    "value": -894743517,
                                    "accum": -894743517,
                                    "repr_data": -0.894743517,
                                    "repr_display": -0.894743517
                                },
                                "stats": {
                                    "sum": 0.0,
                                    "count": 0,
                                    "min": 0.0,
                                    "max": 0.0,
                                    "sqr": 0.0,
                                    "mean": 0.0,
                                    "stddev": 0.0
                                }
                            }
                        },
                        "children": [
                            {
                                "node": {
                                    "hash": 17481650134347108265,
                                    "prefix": "main",
                                    "tid": [
                                        0
                                    ],
                                    "pid": [
                                        2539175
                                    ],
                                    "depth": 1,
                                    "is_dummy": false,
                                    "inclusive": {
                                        "entry": {
                                            "laps": 1,
                                            "value": 894743517,
                                            "accum": 894743517,
                                            "repr_data": 0.894743517,
                                            "repr_display": 0.894743517
                                        },
                                        "stats": {
                                            "sum": 0.894743517,
                                            "count": 1,
                                            "min": 0.894743517,
                                            "max": 0.894743517,
                                            "sqr": 0.8005659612135293,
                                            "mean": 0.894743517,
                                            "stddev": 0.0
                                        }
                                    },
                                    "exclusive": {
                                        "entry": {
                                            "laps": 1,
                                            "value": -1773605,
                                            "accum": -1773605,
                                            "repr_data": -0.001773605,
                                            "repr_display": -0.001773605
                                        },
                                        "stats": {
                                            "sum": -0.001773605,
                                            "count": 1,
                                            "min": 9.22e-07,
                                            "max": 0.896506392,
                                            "sqr": -0.0031577497803754,
                                            "mean": -0.001773605,
                                            "stddev": 0.0
                                        }
                                    }
                                },
                                "children": [
                                    {
                                        "..." : "... etc. ..."
                                    }
                                ]
                            }
                        ]
                    }
                ]
            ]
        }
    }
}
```

#### Timemory JSON Output Python Post-Processing Example

```python
#!/usr/bin/env python3

import sys
import json


def read_json(inp):
    with open(inp, "r") as f:
        return json.load(f)


def find_max(data):
    """Find the max for any function called multiple times"""
    max_entry = None
    for itr in data:
        if itr["entry"]["laps"] == 1:
            continue
        if max_entry is None:
            max_entry = itr
        else:
            if itr["stats"]["mean"] > max_entry["stats"]["mean"]:
                max_entry = itr
    return max_entry


def strip_name(name):
    """Return everything after |_ if it exists"""
    idx = name.index("|_")
    return name if idx is None else name[(idx + 2) :]


if __name__ == "__main__":

    input_data = [[x, read_json(x)] for x in sys.argv[1:]]

    for file, data in input_data:
        for metric, metric_data in data["timemory"].items():

            print(f"[{file}] Found metric: {metric}")

            for n, itr in enumerate(metric_data["ranks"]):

                max_entry = find_max(itr["graph"])
                print(
                    "[{}] Maximum value: '{}' at depth {} was called {}x :: {:.3f} {} (mean = {:.3e} {})".format(
                        file,
                        strip_name(max_entry["prefix"]),
                        max_entry["depth"],
                        max_entry["entry"]["laps"],
                        max_entry["entry"]["repr_data"],
                        metric_data["unit_repr"],
                        max_entry["stats"]["mean"],
                        metric_data["unit_repr"],
                    )
                )
```

This script applied to the corresponding JSON output from [Text Output Example](#timemory-text-output-example) would be:

```console
[openmp-cg.inst-wall_clock.json] Found metric: wall_clock
[openmp-cg.inst-wall_clock.json] Maximum value: 'conj_grad' at depth 6 was called 76x :: 10.641 sec (mean = 1.400e-01 sec)
```
