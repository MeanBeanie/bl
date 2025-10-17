# BL: a simplistic programming language

I got bored with conventional programming languages that have fancy features like classes, so I made BL. It's a statically typed (and by typed I mean you get floats and strings) programming language with support for functions, designed to be somewhat easy to read and type.

# Installation

```sh
$ git clone https://github.com/MeanBeanie/bl.git
$ cd bl
$ make
```

out will come a bouncing, baby binary called `bl` and away you go interpreting your own scripts.

# Usage/Examples

Provided below is a simple hello world script that uses a variable.

```
# oh and comments exist, isn't that neat
str world = ", World!";

sout("Hello", world, "\n");
```

As you can see it uses the builtin `sout` function, which as you could expect takes each argument and sends it to **s**td**out** (wow I wonder where the name `sout` came from)

User created functions are also supported, with the following syntax

```
fun math(num a, num b, num c) = {
	# note that the sout command needs to be passed a newline
	sout(a + b * c, "\n");
}

num x = 100.2;

math(x, 32.2, 64.4);
```

> Note: Functions do not return any values currently, and while they look like variables they cannot be passed around like variables (they're waiting till marriage, you understand)

# Roadmap

> Note: These are listed in **no particular order**, they will be finished as I feel like working on them

- [x] Conditional branching
- [ ] Loops
- [ ] Function return values
- [ ] Standard library
- [ ] Passing around functions as arguments
- [ ] Command line arguments

# Trivia

You may be wondering where the name BL comes from. The actual abbrieviation is upto interpretation but I've found myself partial to **B**asic **L**anguage, and definitely not any other abbrieviation that happens to also be BL
