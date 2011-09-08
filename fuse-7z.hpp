#ifndef FUSE_7ZIP_H
#define FUSE_7ZIP_H

#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <string>
#include <list>
#include <sstream>
#include <iostream>

#ifdef _LFS_LARGEFILE
typedef off64_t offset_t;
#else
typedef off_t offset_t;
#endif

class Node;
class Fuse7z;
class BigBuffer;

extern class Logger {
	public:
	std::stringstream ss;
	Logger();
	~Logger();
	void logger(std::string const & text);
	void err(std::string const & text);
	template<typename T>
	inline Logger & operator<<(T const & data) {
		ss << data;
		return *this;
	}
	static Logger &endl(Logger &f);

	typedef Logger& (*LoggerManipulator)(Logger&);

	    // take in a function with the custom signature
	Logger& operator<<(LoggerManipulator manip)
	{
		// call the function, and return it's value
		return manip(*this);
	}
} logger;

class BigBuffer {
	private:
	//TODO: use >> and <<
	static const unsigned int chunkSize = 4*1024; //4 Kilobytes

	Fuse7z * z;
	class ChunkWrapper;

	typedef std::vector<ChunkWrapper> chunks_t;

	struct CallBackStruct {
		size_t pos;
		const BigBuffer *buf;
		time_t mtime;
	};

	chunks_t chunks;


	public:

	offset_t len;

	/**
	 * Create new file buffer without mapping to file in a zip archive
	 */
	BigBuffer();

	/**
	 * Read file data from file inside zip archive
	 *
	 * \param z         Zip file
	 * \param nodeId    Node index inside zip file
	 * \param length    File length
	 * @throws
	 *      std::exception  On file read error
	 *      std::bad_alloc  On memory insufficiency
	 */
	BigBuffer(struct Fuse7z *z, int nodeId, ssize_t length);

	~BigBuffer();

	/**
	 * Dispatch read requests to chunks of a file and write result to
	 * resulting buffer.
	 * Reading after end of file is not allowed, so 'size' is decreased to
	 * fit file boundaries.
	 *
	 * \param buf       destination buffer
	 * \param size      requested bytes count
	 * \param offset    offset to start reading from
	 * \return always 0
	 */
	int read(char *buf, size_t size, offset_t offset) const;

	/**
	 * Dispatch write request to chunks of a file and grow 'chunks' vector if
	 * necessary.
	 * If 'offset' is after file end, tail of last chunk cleared before growing.
	 *
	 * \param buf       Source buffer
	 * \param size      Number of bytes to be written
	 * \param offset    Offset in file to start writing from
	 * \return number of bytes written
	 * @throws
	 *      std::bad_alloc  If there are no memory for buffer
	 */
	int write(const char *buf, size_t size, offset_t offset);

	/**
	 * Create (or replace) file element in zip file. Class instance should
	 * not be destroyed until zip_close() is called.
	 *
	 * \param mtime     File modification time
	 * \param z         ZIP archive structure
	 * \param fname     File name
	 * \param newFile   Is file not yet created?
	 * \param index     File index in ZIP archive
	 * \return
	 *      0       If successfull
	 *      -ENOMEM If there are no memory
	 */
	int saveToZip(time_t mtime, const char *fname, bool newFile, int index);

	/**
	 * Truncate buffer at position offset.
	 * 1. Free chunks after offset
	 * 2. Resize chunks vector to a new size
	 * 3. Fill data block that made readable by resize with zeroes
	 *
	 * @throws
	 *      std::bad_alloc  If insufficient memory available
	 */
	void truncate(offset_t offset);


	private:



#if 0
	/**
	 * Callback for zip_source_function.
	 * ZIP_SOURCE_CLOSE is not needed to be handled, ZIP_SOURCE_ERROR is
	 * never called because read() always successfull.
	 * See zip_source_function(3) for details.
	 */
	static ssize_t zipUserFunctionCallback(void *state, void *data, size_t len, enum zip_source_cmd cmd);
#endif
	/**
	 * Return number of chunks needed to keep 'offset' bytes.
	 */
	inline static unsigned int chunksCount(offset_t offset) {
		return (offset + chunkSize - 1) / chunkSize;
	}

	/**
	 * Return number of chunk where 'offset'-th byte is located.
	 */
	inline static unsigned int chunkNumber(offset_t offset) {
		return offset / chunkSize;
	}

	/**
	 * Return offset inside chunk to 'offset'-th byte.
	 */
	inline static int chunkOffset(offset_t offset) {
		return offset % chunkSize;
	}

};

class Fuse7z {
	private:
	public:
	std::string const archive_fn;
	std::string const cwd;
	Node * root_node;

	public:
	Fuse7z(std::string const & filename, std::string const & cwd);
	virtual ~Fuse7z();


	virtual void open(char const * path, Node * n) = 0;
	virtual void close(char const * path, Node * n) = 0;
	virtual int read(char const * path, Node * node, char * buf, size_t size, off_t offset) = 0;

};


#endif // FUSE_7ZIP_H
