#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

jmp_buf envHandleSIGBUS;

void abortWith (char const * message, char const * extended, char const * extVar) {
	if (!extended) fprintf (stderr, "%s\n", message);
	else fprintf (stderr, "ERROR: '%s' %s: %s\n", extVar, message, extended);
	exit (-1);
}

void searchWarn (char const * message, char const * file, char const * err, int * rc) {
	fprintf (stderr, "WARNING: '%s' %s: %s\n", file, message, err);
	++optind;
	*rc = -1;
}

void handleSIGBUS () {
	longjmp (envHandleSIGBUS, 0xDEADBEEF);
}

void findMatch (int argc, char ** argv, int * rc, int * matchFlag, char * pattern, int context, int fd) {
	int fdSearch, k, sayYourPrayers;
	char * filePtr, * fileStartPtr, * chkPattern, * i;
	struct stat stat;
	chkPattern = pattern;

	if (fd >= 0) fdSearch = fd;

	while (optind < argc) {
		if (fd < 0 && (fdSearch = open (argv[optind], O_RDONLY, 0444)) < 0) {
			searchWarn ("open() failure [reading]", argv[optind], strerror (errno), rc);
			continue;
		}
		
		if (fstat (fdSearch, &stat) < 0) {
			searchWarn ("fstat() failure", argv[optind], strerror (errno), rc);
			if (fd < 0 && close (fdSearch) < 0) // try to close
				fprintf (stderr, "WARNING: '%s' close() failure: %s\n", argv[optind - 1], strerror (errno));
			continue;
		}

		if ((filePtr = fileStartPtr = mmap (0, stat.st_size, PROT_READ, MAP_SHARED, fdSearch, 0)) < 0) {
			searchWarn ("mmap() failure", argv[optind], strerror (errno), rc);
			if (fd < 0 && close (fdSearch) < 0) // try to close
				fprintf (stderr, "WARNING: '%s' close() failure: %s\n", argv[optind - 1], strerror (errno));
			continue;
		}

		sayYourPrayers = setjmp (envHandleSIGBUS);

		// send prayers of thanks to the signal() and setjmp()/longjmp() gods
		// we switched to Geico and saved 15% or more on stack insurance
		if (sayYourPrayers != 0xDEADBEEF)
			for (; filePtr < fileStartPtr + stat.st_size; ++filePtr)
				if (*filePtr == *chkPattern) {
					++chkPattern;
					if (*chkPattern == '\0') { // there is a match
						*matchFlag = 1;
	
						printf ("%s:%llu\t", (fd < 0) ? argv[optind] : "<standard input>", (unsigned long long int) (filePtr - strlen (pattern) + 1 - fileStartPtr));
	
						for (i = (filePtr - strlen (pattern) + 1 - context); i < (filePtr - strlen (pattern) + 1); ++i)
							if (i >= fileStartPtr) printf (" %c", (*i < 32 || *i > 126) ? 46 : (unsigned char) *i);
						for (k = 0; pattern[k] != '\0'; ++k) printf (" %c", (pattern[k] < 32 || pattern[k] > 126) ? 46 : (unsigned char) pattern[k]);
						for (i = filePtr + 1; i < (filePtr + 1 + context); ++i)
							if (i <= (fileStartPtr + stat.st_size)) printf (" %c", (*i < 32 || *i > 126) ? 46 : (unsigned char) *i);
	
						printf ("\t");

						for (i = (filePtr - strlen (pattern) + 1 - context); i < (filePtr - strlen (pattern) + 1); ++i)
							if (i >= fileStartPtr) printf (" %02x", (unsigned char) *i);
						for (k = 0; pattern[k] != '\0'; ++k) printf (" %x", (unsigned char) pattern[k]);
						for (i = filePtr + 1; i < (filePtr + 1 + context); ++i)
							if (i <= (fileStartPtr + stat.st_size)) printf (" %02x", (unsigned char) *i);

						printf ("\n");

						chkPattern = pattern; // start over
					}
				} else chkPattern = pattern;  // start over
		else
			fprintf (stderr, "\nSIGBUS encountered (bad memory access): munmap()'ing and close()'ing %s...\n", (fd < 0) ? argv[optind] : "<standard input>");

		if (munmap (fileStartPtr, stat.st_size) < 0) { // this should probably be a 'fatal' error...
			searchWarn ("munmap() failure", argv[optind], strerror (errno), rc);
			if (fd < 0 && close (fdSearch) < 0) // try to close
				fprintf (stderr, "WARNING: '%s' close() failure: %s\n", argv[optind - 1], strerror (errno));
			continue;
		}
		
		if (fd < 0 && close (fdSearch) < 0) {
			searchWarn ("close() failure", argv[optind], strerror (errno), rc);
			continue;
		}

		++optind;
	}
}

int main (int argc, char ** argv) {
	char option;
	char * pfPath, * pattern;
	int context, fdPattern, rc, matchFlag;
	struct stat stat;
	pfPath = NULL;
	context = fdPattern = rc = matchFlag = 0;

	if (signal (SIGBUS, handleSIGBUS) == SIG_ERR)
		abortWith ("signal() failure", strerror (errno), pfPath);

	while ((option = getopt (argc, argv, "c:p:")) != -1)
		switch (option) {
			case 'c':
				context = (int) strtol (optarg, NULL, 0);
				break;
			case 'p':
				pfPath = optarg;
				break;
			default:
				abortWith ("Usage: bgrep [-c context_bytes] -p pattern_file [file1 [file2 ...] ]\n       bgrep [-c context_bytes] pattern [file1 [file2 ...] ]", 0, 0);
		}

	if (pfPath != NULL && (fdPattern = open (pfPath, O_RDONLY, 0444)) >= 0) {
		if (fstat (fdPattern, &stat) < 0)
			abortWith ("fstat() failure", strerror (errno), pfPath);

		pattern = (char *) malloc (65536);

		if (read (fdPattern, pattern, stat.st_size) < 0)
			abortWith ("read() failure", strerror (errno), pfPath);

		if (close (fdPattern) < 0)
			abortWith ("close() failure", strerror (errno), pfPath);
	} else if (fdPattern < 0) {
		abortWith ("open() failure [reading]", strerror (errno), pfPath);
	} else {
		if (optind < argc)
			pattern = argv[optind++];
		else
			abortWith ("Usage: bgrep [-c context_bytes] -p pattern_file [file1 [file2 ...] ]\n       bgrep [-c context_bytes] pattern [file1 [file2 ...] ]", 0, 0);
	}

	if (optind < argc) // input files specified
		findMatch (argc, argv, &rc, &matchFlag, pattern, context, -1);
	else               // no input files specified; assume stdin
		findMatch (argc + 1, argv, &rc, &matchFlag, pattern, context, STDIN_FILENO);

	if (rc != -1 && matchFlag != 1) rc = 1; // if no match && no syscall error, return 1
	
	return rc;
}
