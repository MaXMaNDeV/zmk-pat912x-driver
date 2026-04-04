# zmk-pat912x-driver

PAT912x trackball sensor driver for ZMK v0.3 (Zephyr 3.5).

Extracted from Zephyr 4.1 (`zmkfirmware/zephyr` v4.1.0+zmk-fixes) for use with older Zephyr versions that don't include the driver natively.

## Usage

Add to your `west.yml`:

```yaml
manifest:
  remotes:
    - name: MaXMaNDeV
      url-base: https://github.com/MaXMaNDeV
  projects:
    - name: zmk-pat912x-driver
      remote: MaXMaNDeV
      revision: main
```

No overlay changes needed - uses the same `compatible = "pixart,pat912x"` as Zephyr 4.1+.

## Files

- `drivers/input/input_pat912x.c` - Driver source
- `drivers/input/Kconfig.pat912x` - Kconfig
- `dts/bindings/input/pixart,pat912x.yaml` - Device tree binding
- `include/zephyr/input/input_pat912x.h` - Public API header

## License

Apache-2.0 (same as Zephyr)
