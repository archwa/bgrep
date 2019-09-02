# bgrep

A binary implementation of the Unix `grep(1)` utility.

## Description

`bgrep` searches for a binary pattern in one or more input files and provides
information about each occurrence of the pattern within the files.

A couple of options can be provided to `bgrep` to specify its behavior. These
options are described below:

* `-p pattern_file`: Read the pattern from pattern\_file.

* `-c context_bytes`: When a match is found, also output the binary context
surrounding the match for context\_bytes before and after.  If there are fewer
than context\_bytes either before or after, that part of the output is
truncated.


More details are given in the `doc` directory.

## Dependencies

* gcc
* make

## Compilation

To build `bgrep`, simply do the following:

	make bgrep

## Usage

Usage syntax for `bgrep` is given by the following:

	bgrep [options] -p pattern_file [file1 [file2] ...]
	bgrep [options] pattern [file1 [file2] ...]
