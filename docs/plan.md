# Real-Time C++ Perception Pipeline — Project Plan

Oct 3, 2026 · @Adrien

## Product vision

Build a real-time C++20 perception pipeline that detects and tracks an object from a camera and drives a pan-tilt mount through an STM32, with latency measured at every stage. Plan for about 7 weeks; start applying once Phase 3 is done.

**Working name:** `trackpipe` (rename freely).

**Success criteria for v1.0**

- Camera-to-actuator loop works live: the mount follows a target held in front of the camera.
- Per-stage and end-to-end latency reported as p50 / p99, plus FPS and dropped frames, for every backend and precision.
- Inference backend and camera source are switchable by a command-line flag, without recompiling.
- Builds from a clean clone with one command; CI green; ASan and TSan clean.

**Out of scope:** training models, ROS, a GUI beyond a debug overlay, a native Windows build, and hard real-time guarantees on the laptop (hard real-time lives on the STM32).

## Architecture

The laptop runs a five-stage threaded pipeline and sends a target position over USB serial; the STM32 owns the timing-critical control loop and the failsafe.

&#91;embedded content: trackpipe architecture · host pipeline and MCU control loop\]

The dashed arrow closes the loop physically: moving the servos moves the camera, which changes the next frame. Telemetry also flows back from the MCU so the host can measure round-trip latency.

**Interfaces you will keep stable from Phase 1 onwards**

| Interface | Implementations | Chosen by |
| --- | --- | --- |
| `FrameSource` | file, UDP stream, V4L2 | `--source` URI |
| `InferenceBackend` | fake (sleep), TensorRT, ONNX Runtime | `--backend`, `--precision` |
| `FrameSink` | display, file, serial | `--sink` (several allowed) |
| Queue policy | `Block`, `DropOldest` | template parameter, set in config |
| Wire protocol | one header-only library | compiled into both host and firmware |

**Environment constraints and what they change**

| Constraint | Consequence for the design |
| --- | --- |
| WSL2, not native Linux | No real-time guarantees on the host; webcam access may need a network stream |
| Quadro T1000, 4 GB | Nano-class models only; INT8 and FP16 supported (Turing) |
| STM32 Nucleo available | Hard real-time and failsafe live there |

## Phase 0 — Environment and dev container (3–4 days)

**Sprint goal:** a dev container, running on Docker Engine inside WSL2, where CUDA, TensorRT, the Nucleo serial port and an OpenCV window all work, plus a repo skeleton with build presets and CI.

**Backlog**

- [X] Install WSL2 with Ubuntu 24.04. Install only the Windows NVIDIA driver; never install a Linux GPU driver inside WSL. Check that `nvidia-smi` works in WSL.
- [X] Install Docker Engine inside the Ubuntu distro (not Docker Desktop; don't run both), then the NVIDIA Container Toolkit. Check that `docker run --rm --gpus all` with a CUDA base image prints `nvidia-smi` output.
- [X] Keep the repo on the WSL filesystem (for example `~/trackpipe`), never under `/mnt/c`.
- [X] Write a multi-stage `Dockerfile`:
  - `dev-gpu`: based on an NVIDIA NGC TensorRT image, plus GCC 13+ or Clang 18+, CMake 3.25+, Ninja, clang-format, clang-tidy, GDB, OpenCV and GoogleTest. Check that the image's TensorRT supports the T1000 and that your Windows driver is new enough for its CUDA version.
  - `ci-cpu`: slim Ubuntu with the CPU-only dependencies, used by CI and later as the base for the ARM64 cross-build.
- [X] Install `usbipd-win` on Windows; bind and attach the Nucleo (`usbipd list`, `usbipd bind`, `usbipd attach --wsl`) and confirm `/dev/ttyACM0` in WSL. Script the attach step, since it does not survive unplugging or rebooting.
- [X] Try attaching the webcam the same way and check for `/dev/video0`. Timebox this to one afternoon; if it fails, the network source in Phase 1 covers it.
- [X] Enable WSL mirrored networking in `.wslconfig`, so Windows tools can stream to `localhost`.
- [X] Write `devcontainer.json` for VS Code Dev Containers with these run arguments, documented in the README:
  - GPU: `--gpus all`
  - Devices: `--device /dev/ttyACM0` (plus `/dev/video0` if attached); attach devices before starting the container
  - Display: mount `/tmp/.X11-unix` and `/mnt/wslg`, pass `DISPLAY`, `WAYLAND_DISPLAY` and `XDG_RUNTIME_DIR`
  - Real-time: `--cap-add=SYS_NICE --ulimit rtprio=99`
  - Network: `--network host`, or publish the UDP port (`-p 5000:5000/udp`)
- [X] Inside the container, run `deviceQuery` and `trtexec` on a small ONNX model to confirm the GPU stack end to end.
- [X] Add `CMakePresets.json`: `debug`, `debug-asan` (ASan + UBSan), `debug-tsan`, `release`.
- [X] GitHub Actions workflow that builds the `ci-cpu` image, builds the project and runs tests.
- [X] Flash the STM32 outside the container: STM32CubeProgrammer on Windows, or `st-flash`/OpenOCD in WSL.
- [X] Order hardware now so it arrives by Phase 4: two micro servos with a pan-tilt bracket, a separate 5 V supply, jumper wires, and ideally a small USB webcam to mount on the bracket.
- [X] Record two or three test clips (a person or object moving at different speeds) with the Windows camera app.

**Acceptance criteria**

- `nvidia-smi` and `trtexec` run inside the dev container.
- From inside the container, writing to `/dev/ttyACM0` reaches the Nucleo (verify with an existing UART echo firmware).
- A test OpenCV window opens from the container through WSLg.
- An empty "hello" target builds with every preset in the dev container, and in CI with the `ci-cpu` image.
- From a fresh clone, "Reopen in Container" gives a working build with no manual steps.

**Watch out for:** NGC images are large, so check free space on the WSL virtual disk. `--device` fails if the device is missing when the container starts. Profilers have partial support inside WSL2 and need extra privileges in a container, so note what you can and cannot measure rather than fighting it.

## Phase 1 — Skeleton: sources, sinks, build (week 1)

**Sprint goal:** frames flow from any source to a display or file, single-threaded, through clean interfaces you will keep for the rest of the project.

**User story:** as a developer, I can run `trackpipe --source file:clip.mp4 --sink display` and swap `file:` for `udp://` or `v4l2:/dev/video0` without touching code.

**Backlog**

- [X] Define `Frame`: width, height, stride, pixel format, sequence number, capture timestamp (`std::chrono::steady_clock`), and an owned pixel buffer. Make it move-only.
- [ ] Define the `FrameSource` interface: `open()`, `next() -> std::expected<Frame, Error>`, `close()`.
- [ ] `FileSource`: decode a video file (OpenCV or FFmpeg's libav). Add a `--realtime` option that paces frames at the file's FPS, so benchmarks behave like a camera.
- [ ] `NetworkSource`: receive an MPEG-TS stream over UDP. On Windows, stream the webcam with `ffmpeg -f dshow -i video="<camera name>" -f mpegts udp://localhost:5000`.
- [ ] `V4l2Source` (if the webcam attached in Phase 0): open the device, negotiate format, `mmap` buffers, queue/dequeue. Wrap the file descriptor and each mapping in RAII types.
- [ ] Sinks: `DisplaySink` (window with a debug overlay showing FPS) and `FileSink` (writes an annotated video).
- [ ] Command-line parsing (CLI11 or your own small parser) and a factory that builds sources and sinks from URI-like strings.
- [ ] Unit tests with GoogleTest: `Frame` move semantics, `FileSource` returns the expected frame count, a malformed URI returns an error instead of throwing.

**Acceptance criteria**

- The same binary runs with the file source and the network source and shows video at the source frame rate.
- Clean under the `debug-asan` preset; CI green.
- No raw `new`/`delete`; every OS resource (fd, `mmap`, socket) is owned by an RAII type.

**C++ focus:** RAII, rule of zero vs rule of five, move-only types, `std::expected` for errors, `std::span` for buffer views, `enum class`, and modern CMake (one library target, one executable, one test target, `target_link_libraries` with `PRIVATE`/`PUBLIC`).

**Be ready to explain in an interview:** why `Frame` is move-only, why `std::expected` instead of exceptions on the hot path, and what happens to your V4L2 buffers if `open()` fails halfway through.

## Phase 2 — Concurrent pipeline (weeks 2–3)

**Sprint goal:** each stage runs on its own thread, connected by lock-free bounded queues, with no heap allocation per frame and latency measured end to end. This is the phase interviewers will dig into most.

**User story:** as an operator, when inference is slower than the camera, the pipeline stays responsive and shows the newest frame instead of falling further and further behind.

**Backlog**

- [ ] Split the pipeline into stages: capture → preprocess → inference → postprocess and track → sink. One `std::jthread` per stage; shut down with `std::stop_token`.
- [ ] Write a single-producer/single-consumer ring buffer: `template <typename T, std::size_t N> class SpscQueue`. Use `std::atomic` head and tail indices with acquire/release ordering, and pad them onto separate cache lines (`alignas(64)`).
- [ ] Make the full-queue behaviour a policy: `Block` (throughput-first) or `DropOldest` (latency-first). Constrain `T` with a concept.
- [ ] Add a frame buffer pool: buffers are pre-allocated and recycled through a return queue, so steady state allocates nothing.
- [ ] Add a `FakeInference` stage that sleeps for a configurable time, to test backpressure before a real model exists.
- [ ] Write the preprocess stage yourself: resize, letterbox, convert BGR to RGB, normalize to a float NCHW tensor. Benchmark it against OpenCV's equivalent.
- [ ] Instrumentation: each stage stamps entry and exit times into the frame's metadata. Collect a latency histogram per stage and end to end; print p50/p95/p99, FPS and drops per stage; export CSV.
- [ ] Google Benchmark micro-benchmarks: your SPSC queue vs a mutex + `std::queue`, and with vs without cache-line padding (to see false sharing).
- [ ] Tests: a stress test pushing millions of items through the queue under the `debug-tsan` preset.

**Acceptance criteria**

- TSan reports nothing on the stress test or on a full pipeline run.
- With a 30 FPS source and `FakeInference` set to 50 ms, `DropOldest` keeps end-to-end latency bounded while `Block` latency grows. Show both on one plot in the README.
- Zero heap allocations per frame in steady state (verify with a counting allocator or `heaptrack`).
- Ctrl-C stops all threads cleanly, with no leaked buffers.

**C++ focus:** the C++ memory model (relaxed vs acquire/release vs sequential consistency), `std::jthread` and `std::stop_token`, policy-based design with templates, concepts, `std::chrono`, and custom allocators.

**Be ready to explain:** why acquire/release is enough for SPSC but not for multiple producers, what false sharing is and how you measured it, and why you chose drop-oldest for a control application.

## Phase 3 — Inference backends and quantization (weeks 4–5)

**Sprint goal:** a real detector runs behind one interface on two backends (TensorRT on the T1000, ONNX Runtime on the CPU), with a measured comparison of FP32, FP16 and INT8.

**User story:** as an integrator, I pick `--backend tensorrt --precision int8` or `--backend onnxruntime` at launch and get the same detections format either way.

**Backlog**

- [ ] Pick a nano-class detector (for example a YOLO "n" variant) and export it to ONNX with a fixed input size. Check the model's license before publishing weights or demos.
- [ ] Define `InferenceBackend`: `load(model_path, options)`, `infer(TensorView input) -> std::expected<RawOutputs, Error>`. Select the implementation through a factory.
- [ ] Hide TensorRT and CUDA headers behind the pimpl idiom, and add a CMake option so the project still builds without CUDA (needed for CI and ARM64).
- [ ] `TensorRtBackend`: build the engine from ONNX with the builder API, serialize it to a cache file, and reload it on later runs. Use a CUDA stream, pinned host memory and asynchronous copies.
- [ ] INT8: implement a calibrator class in C++ that feeds a few hundred representative frames from your clips.
- [ ] `OnnxRuntimeBackend` on the CPU; set the thread count explicitly and record it in results.
- [ ] Postprocessing in C++: decode the output tensor into boxes and write your own non-maximum suppression (NMS).
- [ ] Optional stretch goal: a CUDA kernel for the preprocess step (resize + normalize), compared with the CPU version.
- [ ] Benchmark matrix, scripted: backend × precision → per-stage p50/p99 latency, FPS, and accuracy on a small labelled set (Python with pycocotools is fine for scoring).
- [ ] Capture one Nsight Systems trace showing whether copies overlap with compute.

**Acceptance criteria**

- Switching backend or precision needs no recompilation.
- The results table is in the README, with the INT8 accuracy change and a short explanation of where it comes from.
- The engine cache is invalidated when the model or precision changes.
- The CPU-only build passes CI.

**C++ focus:** interface design (virtual dispatch vs templates, and why runtime selection forces virtual dispatch here), pimpl as a compile firewall, factories, RAII wrappers for CUDA resources (streams, device buffers), and ownership across an API boundary.

**Be ready to explain:** why TensorRT engines are not portable between GPUs, how INT8 calibration chooses scales, what per-channel vs per-tensor quantization means, and why preprocessing can dominate latency on small models.

**Gate:** this is the point to start applying. Update the CV with the project and link the repo.

## Phase 4 — Tracking and STM32 closed loop (week 6)

**Sprint goal:** the host tracks one target and streams its position to the Nucleo, which runs a FreeRTOS control loop that drives the pan-tilt servos and fails safe when messages stop.

**Setup choice:** mount a small USB webcam on the pan-tilt bracket. Then the control error is simply the target's offset from the image centre, which makes this true visual servoing. If you only use the laptop's built-in camera, the mount points at the target instead, which needs a calibrated mapping from pixels to angles and is open loop.

**Backlog — host side**

- [ ] Target selection: one class, highest confidence, with hysteresis so the target does not jump between objects.
- [ ] Constant-velocity Kalman filter in image coordinates (state: x, y, vx, vy) that keeps predicting through missed detections.
- [ ] Latency compensation: predict the target forward by the measured pipeline latency before sending it. This is a direct tie-in to your control background.
- [ ] Write a small fixed-size matrix template (`Matrix<T, R, C>` with `constexpr` operations) for the filter, then compare it with Eigen.
- [ ] `SerialSink`: an RAII `termios` wrapper, its own thread, non-blocking writes.

**Backlog — shared protocol**

- [ ] A header-only C++ protocol library compiled for both the host and the STM32: no heap, no exceptions, no RTTI. Frame format: start byte, length, message ID, payload, CRC-16.
- [ ] Messages: target error (pan, tilt), sequence number, host timestamp; telemetry back from the MCU (loop period, last sequence received).
- [ ] Host unit tests for the parser, including a fuzz test feeding random bytes: it must never crash or accept a bad CRC.

**Backlog — STM32 firmware**

- [ ] UART receive via DMA with idle-line interrupt, handing complete frames to a parser task.
- [ ] A fixed-rate control task: PI controller per axis, output through timer PWM at 50 Hz servo frequency.
- [ ] Failsafe: if no valid message arrives within a timeout, hold or return to centre.
- [ ] Telemetry task reporting loop timing, so the host can compute round-trip latency.

**Acceptance criteria**

- The mount follows a target moved by hand in front of the camera.
- Unplugging the USB cable triggers the failsafe within the configured timeout.
- The parser fuzz test passes, and the same protocol code compiles in both builds.
- Servos run from the separate 5 V supply with a common ground, never from the Nucleo's 5 V pin.

**C++ focus:** templates and `constexpr`, embedded C++ constraints (`-fno-exceptions -fno-rtti`, no dynamic allocation), `std::array` and `std::span` on the MCU, and code shared between two targets.

**Be ready to explain:** why hard real-time sits on the MCU and soft real-time on the host, how latency compensation changes tracking quality, and how your framing recovers from a corrupted byte.

## Phase 5 — ARM64, latency analysis and release (week 7)

**Sprint goal:** Package everything for recruiters.

**Backlog — release**

- [ ] README: one-paragraph pitch, demo GIF at the top, architecture diagram, results tables, build instructions for each preset.
- [ ] Short design-decision notes (one page each): queue policy, backend interface, protocol framing, real-time split.
- [ ] A 60–90 second demo video: tracking live, then the latency overlay, then the failsafe.
- [ ] Tag `v1.0`.

**Acceptance criteria**

- Every number in the README can be reproduced with a script in the repo.
- Someone unfamiliar with the project understands what it does from the first screen of the README.

**C++ focus:** CMake toolchain files and presets, writing portable code (no x86-only assumptions, fixed-width integer types), and profiling-driven optimization.

## Timeline, risks and deliverables

Seven weeks at a steady pace, with the job-search gate after Phase 3; Phases 4 and 5 continue while you apply.

&#91;embedded content: roadmap · 6 phases, 2 gates\]

Weeks are planning estimates, not commitments: re-plan at the end of each phase using what actually took longer.

**Risks and mitigations**

| Risk | Mitigation |
| --- | --- |
| Webcam cannot be attached in WSL2 | Network source from Phase 1; file source for all benchmarks |
| TensorRT version does not support the T1000 or driver | Check the support matrix in Phase 0; ONNX Runtime with its CUDA provider as fallback |
| Phase 2 overruns on concurrency bugs | TSan from day one; test backpressure with `FakeInference` before a real model; ship `Block` first, then `DropOldest` |
| Scope creep | CUDA preprocess kernel, Eigen comparison and the RT kernel are optional; skip them if behind |
| Hardware arrives late | Host tracking and protocol tests need no hardware; test serial with a UART loopback on the Nucleo |
| Servo current resets the Nucleo | Separate 5 V supply with a common ground |

**Deliverables for the job search**

- [ ] Public repo with README, results tables, design notes and demo video.
- [ ] One CV entry, filled in with your measured numbers. Example shape: "Real-time C++20 perception pipeline: lock-free SPSC queues, zero-allocation frame pool, TensorRT FP16/INT8 and ONNX Runtime backends; p99 end-to-end latency of N ms on a Turing GPU."
- [ ] A second line for firmware roles: "Closed the loop to an STM32 FreeRTOS controller over a CRC-framed protocol shared between host and firmware."
- [ ] A one-page interview cheat sheet built from each phase's "Be ready to explain" points.
