# DC

A RPN, arbitrary precision calculator.

It's basically my version of `dc` utility, found on most unix-like systems.

I did not look at the original codebase, so everything here is my reimaginantion, based on original programs behavior.

## Usage

The exact same, as the original utility. I was going off of [this document](https://www.gnu.org/software/bc/manual/dc-1.05/html_mono/dc.html), excluding the GNU extension stuff.

## Building

This project depends on `pkg-config` and `gmp` (GNU Multiple Precision Arithmetic Library).

To build the project, you can use the included `makefile`. The available targets are:

```bash
make repl # builds the dc executable
make lib  # builds the static library
make      # builds everything
```

## License

See [LICENSE](LICENSE)
