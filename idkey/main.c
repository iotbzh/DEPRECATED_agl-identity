#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define EXIT_SUCCESS		0
#define EXIT_CMDLINE		1
#define EXIT_FILEOPEN		2
#define EXIT_FILEREAD		3
#define EXIT_FILEWRITE		4
#define EXIT_INVALID		5
#define EXIT_ALLOC			6

#define BLOCK_SIZE			4096

/// @brief Header of the datas.
typedef struct header_
{
	char mn[4];
	size_t size;
} header;

/// @brief Check if the @c v is a valid magick number.
/// @param[in] v magick number.
/// @return 1 if valid, zero otherwise.
int is_valid_nm(const char* v)
{
	return v && v[0] == 'I' && v[1] == 'D' && v[2] == 'K' && v[3] == 'Y';
}

/// @brief Close the file descriptor if valid then print a formatted error message then return the specified code.
/// @param[in] fd File descriptor to close.
/// @param[in] code The exit code to return.
/// @param[in] format The message to print.
/// @param[in] ... Values for formatting.
/// @return The exit code provided by @c code.
int close_and_fail(int fd, int code, const char* format, ...)
{
	va_list arglist;
	fprintf(stderr, "Error: ");
	va_start(arglist, format);
	vfprintf(stderr, format, arglist);
	va_end(arglist);
	fprintf(stderr, "\n");
	return code;
}

/// @brief Read the device @c devname and print it's data to stdout.
/// @param[in] devname Device's name.
/// @return Exit code, zero if success.
int read_device(const char* devname)
{
	int fd = open(devname, O_RDONLY);
	if (fd == -1) return close_and_fail(fd, EXIT_FILEOPEN, "Failed to open '%s'!", devname);

	header h;
	ssize_t sz = read(fd, &h, sizeof(header));
	if (sz != sizeof(header)) return close_and_fail(fd, EXIT_FILEREAD, "Failed to read the header!");
	if (!is_valid_nm(h.mn)) return close_and_fail(fd, EXIT_INVALID, "Not a valid identity key!");

	while(h.size)
	{
		char datas[BLOCK_SIZE + 1];
		memset(datas, BLOCK_SIZE +1, 0);
		size_t count = BLOCK_SIZE > h.size ? h.size : BLOCK_SIZE;

		sz = read(fd, datas, count);

		if (sz != count) close_and_fail(fd, EXIT_FILEREAD, "Failed to read a data block!");
		h.size = (h.size - count);

		printf(datas);
	}
	printf("\n");
	close(fd);
	return EXIT_SUCCESS;
}

/// @brief Write the specified data to the specified device.
/// @param[in] devname Name of the device.
/// @param[in] datas Datas to write.
/// @return Exit code, zero if success, non-zero otherwise.
int write_device(const char* devname, const char* datas)
{
	header h = {
		.mn = {'I', 'D', 'K', 'Y'},
		.size = strlen(datas)
	};
	if (h.size < 1) return close_and_fail(-1, EXIT_CMDLINE, "No data to write!");

	int fd = open(devname, O_WRONLY);
	if (fd == -1) return close_and_fail(fd, EXIT_FILEOPEN, "Failed to open device '%s'!", devname);
	if (write(fd, &h, sizeof(header)) != sizeof(header)) return close_and_fail(fd, EXIT_FILEWRITE, "Failed to write the header!");
	if (write(fd, datas, h.size) != h.size) return close_and_fail(fd, EXIT_FILEWRITE, "Failed to write datas!");
	
	close(fd);
	return EXIT_SUCCESS;
}

/// @brief Entry point.
/// @param[in] argc Number of arguments in @c argv.
/// @param[in] argv Arguments array.
/// @return Exit code, zero if success, non-zero otherwise.
int main(int argc, char** argv)
{
	switch(argc)
	{
	case 0:
	case 1:
		fprintf(stderr, "ERROR: too few arguments!\n");
		return EXIT_CMDLINE;

	case 2: // Read the device
		return read_device(argv[1]);

	case 3: // Write the device
		return write_device(argv[1], argv[2]);

	default:
		fprintf(stderr, "ERROR: too many arguments!\n");
		return EXIT_CMDLINE;
	}
}

