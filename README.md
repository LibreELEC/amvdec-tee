# amvdec-tee

Secure World (TEE) AMRISC firmware pre-load service

Newer Amlogic SoCs (S4 and later) only run signed video decoder (AMRISC)
microcode, which the secure world decrypts and loads. `amvdec-tee` passes
the signed `video_ucode.bin` package to the Amlogic video firmware trusted
application (UUID `526fc4fc-7ee6-4a12-96e3-83da9565bce8`) once per boot.
The kernel decoder driver then asks the secure monitor to load the image
for each codec into the AMRISC.

## Requirements

- An OP-TEE kernel driver and a running `tee-supplicant`, with `libteec`.
- The video firmware TA for the SoC in the `tee-supplicant` TA path.
- A signed `video_ucode.bin` package that the TA accepts.
- On the Amlogic OP-TEE builds tested, the kernel must report that the
  normal world has booted (vendor fast call `0xb200e060`) before the TA
  can be loaded.

## Build

    make
    make DESTDIR=/path/to/rootfs install

`CC`, `CFLAGS`, `LDFLAGS`, `PREFIX` and `DESTDIR` follow the usual
conventions.

## Usage

    amvdec-tee /lib/firmware/meson/vdec/video_ucode.bin

The whole file is sent to the TA in 128 KiB chunks. It exits 0 once the
TA has accepted the package, and non-zero with the TEE error code
otherwise.
