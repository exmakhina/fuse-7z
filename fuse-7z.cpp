#define STANDARD_BLOCK_SIZE (512)
#define ERROR_STR_BUF_LEN 0x100

#include "fuse-7z.hpp"
#include "fuse-7z-node.hpp"

#include <fuse.h>
#include <unistd.h>
#include <limits.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <cerrno>
#include <climits>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <linux/limits.h>
#include <queue>
#include <iostream>
#include <cassert>
#include <sstream>
#include <stdexcept>

using namespace std;

struct Logger logger;

//static int param = 0;
extern "C" int param;

Logger::Logger() {
	ss.str("");
	openlog(PROGRAM, LOG_PID, LOG_USER);
}

Logger::~Logger() {
	closelog();
}

void Logger::logger(std::string const & text) {
	if (param) {
		syslog(LOG_INFO, "%s", text.c_str());
	}
	else {
		cout << "fuse-7z: " << text << std::endl;
	}
	ss.str("");
}
void Logger::err(std::string const & text) {
	if (param) {
		syslog(LOG_ERR, "%s", text.c_str());
	}
	else {
		cerr << text << std::endl;
	}
}

Logger & Logger::endl(Logger &f) {
	f.logger(f.ss.str());
	return f;
}

/**
	* Class that keep chunk of file data.
	*/
class BigBuffer::ChunkWrapper {
	private:
	/**
	* Pointer that keeps data for chunk. Can be NULL.
	*/
	char *m_ptr;

	public:
	/**
	* By default internal buffer is NULL, so this can be used for creating
	* sparse files.
	*/
	ChunkWrapper(): m_ptr(NULL) {
	}

	/**
	* Take ownership on internal pointer from 'other' object.
	*/
	ChunkWrapper(const ChunkWrapper &other) {
		m_ptr = other.m_ptr;
		const_cast<ChunkWrapper*>(&other)->m_ptr = NULL;
	}

	/**
	 * Free pointer if allocated.
	 */
	~ChunkWrapper() {
		if (m_ptr != NULL) {
			free(m_ptr);
		}
	}

	/**
	 * Take ownership on internal pointer from 'other' object.
	 */
	ChunkWrapper &operator=(const ChunkWrapper &other) {
		if (&other != this) {
			m_ptr = other.m_ptr;
			const_cast<ChunkWrapper*>(&other)->m_ptr = NULL;
		}
		return *this;
	}

	/**
	 * Return pointer to internal storage and initialize it if needed.
	 * @throws
	 *      std::bad_alloc  If memory can not be allocated
	 */
	char *ptr(bool init = false) {
		if (init && m_ptr == NULL) {
			m_ptr = (char *)malloc(chunkSize);
			if (m_ptr == NULL) {
				throw std::bad_alloc();
			}
		}
		return m_ptr;
	}

	/**
	 * Fill 'dest' with internal buffer content.
	 * If m_ptr is NULL, destination bytes is zeroed.
	 *
	 * @param dest      Destination buffer.
	 * @param offset    Offset in internal buffer to start reading from.
	 * @param count     Number of bytes to be read.
	 *
	 * @return  Number of bytes actually read. It can differ with 'count'
	 *      if offset+count>chunkSize.
	 */
	size_t read(char *dest, offset_t offset, size_t count) const {
		if (offset + count > chunkSize) {
			count = chunkSize - offset;
		}
		if (m_ptr != NULL) {
			memcpy(dest, m_ptr + offset, count);
		} else {
			memset(dest, 0, count);
		}
		return count;
	}

	/**
	 * Fill internal buffer with bytes from 'src'.
	 * If m_ptr is NULL, memory for buffer is malloc()-ed and then head of
	 * allocated space is zeroed. After that byte copying is performed.
	 *
	 * @param src       Source buffer.
	 * @param offset    Offset in internal buffer to start writting from.
	 * @param count     Number of bytes to be written.
	 *
	 * @return  Number of bytes actually written. It can differ with
	 *      'count' if offset+count>chunkSize.
	 * @throws
	 *      std::bad_alloc  If there are no memory for buffer
	 */
	size_t write(const char *src, offset_t offset, size_t count) {
		if (offset + count > chunkSize) {
			count = chunkSize - offset;
		}
		if (m_ptr == NULL) {
			m_ptr = (char *)malloc(chunkSize);
			if (m_ptr == NULL) {
				throw std::bad_alloc();
			}
			if (offset > 0) {
				memset(m_ptr, 0, offset);
			}
		}
		memcpy(m_ptr + offset, src, count);
		return count;
	}

	/**
	 * Clear tail of internal buffer with zeroes starting from 'offset'.
	 */
	void clearTail(offset_t offset) {
		if (m_ptr != NULL && offset < chunkSize) {
			memset(m_ptr + offset, 0, chunkSize - offset);
		}
	}

};

BigBuffer::BigBuffer(): len(0) {
}

BigBuffer::BigBuffer(Fuse7z *z, int nodeId, ssize_t length): len(length), z(z) {

	//struct zip_file *zf = zip_fopen_index(z, nodeId, 0);
	assert(z != NULL);
	chunks.resize(chunksCount(length), ChunkWrapper());
	unsigned int chunk = 0;
	ssize_t nr;
	while (length > 0) {
		//nr = zip_fread(zf, chunks[chunk].ptr(true), chunkSize);
		if (nr < 0) {
			//zip_fclose(zf);
			throw std::runtime_error("could not create big buffer 1");
		}
		++chunk;
		length -= nr;
	}
	if (0/*zip_fclose(zf)*/) {
		throw std::runtime_error("could not create big buffer 2");
	}
}

BigBuffer::~BigBuffer() {
}

int BigBuffer::read(char *buf, size_t size, offset_t offset) const {
	if (offset > len) {
		return 0;
	}
	int chunk = chunkNumber(offset);
	int pos = chunkOffset(offset);
	if (size > unsigned(len - offset)) {
		size = len - offset;
	}
	int nread = size;
	while (size > 0) {
		size_t r = chunks[chunk].read(buf, pos, size);

		size -= r;
		buf += r;
		++chunk;
		pos = 0;
	}
	return nread;
}

int BigBuffer::write(const char *buf, size_t size, offset_t offset) {
	int chunk = chunkNumber(offset);
	int pos = chunkOffset(offset);
	int nwritten = size;

	if (offset > len) {
		if (len > 0) {
			chunks[chunkNumber(len)].clearTail(chunkOffset(len));
		}
		len = size + offset;
	} else if (size > unsigned(len - offset)) {
		len = size + offset;
	}
	chunks.resize(chunksCount(len));
	while (size > 0) {
		size_t w = chunks[chunk].write(buf, pos, size);

		size -= w;
		buf += w;
		++ chunk;
		pos = 0;
	}
	return nwritten;
}

void BigBuffer::truncate(offset_t offset) {
	chunks.resize(chunksCount(offset));

	if (offset > len && len > 0) {
		// Fill end of last non-empty chunk with zeroes
		chunks[chunkNumber(len)].clearTail(chunkOffset(len));
	}

	len = offset;
}

#if 0
ssize_t BigBuffer::zipUserFunctionCallback(void *state, void *data, size_t len, enum zip_source_cmd cmd) {
	CallBackStruct *b = (CallBackStruct*)state;
	switch (cmd) {
		case ZIP_SOURCE_OPEN: {
			b->pos = 0;
			return 0;
		}
		case ZIP_SOURCE_READ: {
			int r = b->buf->read((char*)data, len, b->pos);
			b->pos += r;
			return r;
		}
		case ZIP_SOURCE_STAT: {
			struct zip_stat *st = (struct zip_stat*)data;
			zip_stat_init(st);
			st->size = b->buf->len;
			st->mtime = b->mtime;
			return sizeof(struct zip_stat);
		}
		case ZIP_SOURCE_FREE: {
			delete b;
			return 0;
		}
		default: {
			return 0;
		}
	}
}
#endif

int BigBuffer::saveToZip(time_t mtime, const char *fname, bool newFile, int index) {
	struct zip_source *s;
	struct CallBackStruct *cbs = new CallBackStruct();
	cbs->buf = this;
	cbs->mtime = mtime;
#if 0
	if ((s=zip_source_function(z, zipUserFunctionCallback, cbs)) == NULL) {
		delete cbs;
		return -ENOMEM;
	}
	if ((newFile && zip_add(z, fname, s) < 0) || (!newFile && zip_replace(z, index, s) < 0)) {
		delete cbs;
		zip_source_free(s);
		return -ENOMEM;
	}
#endif
	return 0;
}

inline Fuse7z *get_data() {
	return (Fuse7z*)fuse_get_context()->private_data;
}

Fuse7z::Fuse7z(std::string const & archiveName, std::string const & cwd) :
 archive_fn(archiveName), cwd(cwd) {
	root_node = new Node(NULL, "");
	root_node->is_dir = true;
}

Fuse7z::~Fuse7z() {
#if defined(FUSE7Z_WRITE)
	if (chdir(cwd.c_str()) != 0) {
		logger.err(std::string("Unable to chdir() to archive directory ") + cwd + ". Trying to save file into /tmp");
		if (chdir(getenv("TMP")) != 0) {
			chdir("/tmp");
		}
	}
#endif
	delete root_node;
}

#if defined(FUSE7Z_WRITE)
int Fuse7z::removeNode(Node *node) const {
	int id = node->id;
	delete node;
	if (id >= 0) {
		return (zip_delete (m_zip, id) == 0)? 0 : ENOENT;
	} else {
		return 0;
	}
}
#endif

#if 0
		stat.st_mtime = time(NULL);
		stat.st_size = 0;
#endif

extern "C" {

void *fuse7z_init(struct fuse_conn_info *conn) {
	   (void) conn;
	   Fuse7z *data = get_data();

	   //logger.logger(std::string("Mounting file system on ") + data->m_archiveName + " (cwd:" + data->cwd + ")");
	   return data;
}

void fuse7z_destroy(void *_data) {
	   Fuse7z *data = (Fuse7z *)_data;
	   // Saving changed data
#if 0
	   for (filemap_t::const_iterator i = data->files.begin(); i != data->files.end(); ++i) {
	       if (i->second->isChanged() && !i->second->is_dir) {
	           int res = i->second->save();
	           if (res != 0) {
	               syslogger(LOG_ERR, "Error while saving file %s in ZIP archive: %d", i->second->full_name.c_str(), res);
	           }
	       }
	   }
#endif
	   delete data;
	   logger << "File system unmounted" << Logger::endl;
}

int fuse7z_getattr(const char *path, struct stat *stbuf) {
	Fuse7z *data = get_data();
	
	if (*path == '\0') {
		return -ENOENT;
	}

	Node * node = data->root_node->find(path + 1);
	if (node == NULL) {
		return -ENOENT;
	}
	
	//logger << "Getattr " << node->fullname() << Logger::endl;

	memcpy(stbuf, &node->stat, sizeof(struct stat));

	if (node->is_dir) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2 + node->childs.size();
		stbuf->st_size = node->childs.size();
	} else {
		stbuf->st_mode = S_IFREG | 0644;
		stbuf->st_nlink = 1;
	}

	//stbuf->st_mtime = time(NULL
	stbuf->st_blksize = STANDARD_BLOCK_SIZE;
	stbuf->st_blocks = (stbuf->st_size + STANDARD_BLOCK_SIZE - 1) / STANDARD_BLOCK_SIZE;
	stbuf->st_uid = geteuid();
	stbuf->st_gid = getegid();

	return 0;
}

int fuse7z_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	Fuse7z *data = get_data();

	if (*path == '\0') {
		return -ENOENT;
	}
	
	//logger << "Reading directory[" << path << "]" << Logger::endl;
	
	Node * node = data->root_node->find(path + 1);
	if (node == NULL) {
		return -ENOENT;
	}

	//logger.logger(node->fullname());

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	for (nodelist_t::const_iterator i = node->childs.begin(); i != node->childs.end(); ++i) {
		Node * node = i->second;
		filler(buf, node->name, NULL, 0);
	}

	return 0;
}

int fuse7z_statfs(const char *path, struct statvfs *buf) {
	(void) path;

	// Getting amount of free space in directory with archive
	struct statvfs st;
	int err;
	if ((err = statvfs(get_data()->cwd.c_str(), &st)) != 0) {
		return -err;
	}
	buf->f_bavail = buf->f_bfree = st.f_frsize * st.f_bavail;

	buf->f_bsize = 1;
	//TODO: may be append archive size?
	buf->f_blocks = buf->f_bavail + 0;

	buf->f_ffree = 0;
	buf->f_favail = 0;

	buf->f_files = -1; // TODO get_data()->files.size() - 1;
	buf->f_namemax = 255;

	return 0;
}

int fuse7z_open(const char *path, struct fuse_file_info *fi) {
	Fuse7z *data = get_data();
	if (*path == '\0') {
		return -ENOENT;
	}
	Node *node = data->root_node->find(path + 1);
	if (node == NULL) {
		return -ENOENT;
	}
	if (node->is_dir) {
		return -EISDIR;
	}
	fi->fh = (uint64_t)node;

	try {
		data->open(path, node);
		return 0;
	}
	catch (std::bad_alloc&) {
		return -ENOMEM;
	}
	catch (std::runtime_error&) {
		return -EIO;
	}
}

int fuse7z_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
#if 0
	(void) mode;

	if (*path == '\0') {
		return -EACCES;
	}
	Node *node = data->root_node->find(path + 1);
	if (node != NULL) {
		return -EEXIST;
	}
	node = new Node(get_data(), path + 1);
	if (!node) {
		return -ENOMEM;
	}
	fi->fh = (uint64_t)node;

	return node->open();
#endif
	return -ENOTSUP;

}

int fuse7z_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	Fuse7z *data = get_data();
	return data->read(path, (Node*)fi->fh, buf, size, offset);
}

int fuse7z_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
#if 0
	(void) path;

	return ((Node*)fi->fh)->write(buf, size, offset);
#endif
	return -ENOTSUP;
}

int fuse7z_release (const char *path, struct fuse_file_info *fi) {
	Fuse7z *data = get_data();
	try {
		data->close(path, (Node*)fi->fh);
		return 0;
	}
	catch(...) {
		return -ENOMEM;
	}
}

int fuse7z_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi) {
#if 0
	(void) path;

	return -((Node*)fi->fh)->truncate(offset);
#endif
	return -ENOTSUP;
}

int fuse7z_truncate(const char *path, off_t offset) {
#if 0
	if (*path == '\0') {
		return -EACCES;
	}
	Node *node = data->root_node->find(path + 1);
	if (node == NULL) {
		return -ENOENT;
	}
	if (node->is_dir) {
		return -EISDIR;
	}
	int res;
	if ((res = node->open()) != 0) {
		return res;
	}
	if ((res = node->truncate(offset)) != 0) {
		node->close();
		return -res;
	}
	return node->close();
#endif
	return -ENOTSUP;
}

int fuse7z_unlink(const char *path) {
#if 0
	if (*path == '\0') {
		return -ENOENT;
	}
	Node *node = data->root_node->find(path + 1);
	if (node == NULL) {
		return -ENOENT;
	}
	if (node->is_dir) {
		return -EISDIR;
	}
	return -get_data()->removeNode(node);
#endif
	return -ENOTSUP;
}

int fuse7z_rmdir(const char *path) {
#if 0
	if (*path == '\0') {
		return -ENOENT;
	}
	Node *node = data->root_node->find(path + 1);
	if (node == NULL) {
		return -ENOENT;
	}
	if (!node->is_dir) {
		return -ENOTDIR;
	}
	if (!node->childs.empty()) {
		return -ENOTEMPTY;
	}
	return -get_data()->removeNode(node);
#endif
	return -ENOTSUP;
}

int fuse7z_mkdir(const char *path, mode_t mode) {
#if 0
	Fuse7z * data = fuse_get_context()->private_data;

	(void) mode;
	if (*path == '\0') {
		return -ENOENT;
	}
	int idx = zip_add_dir(get_zip(), path + 1);
	if (idx < 0) {
		return -ENOMEM;
	}
	Node *node = new Node(get_data(), path + 1, idx);
	if (!node) {
		return -ENOMEM;
	}
	node->is_dir = true;
	return 0;
#endif
	return -ENOTSUP;
}

int fuse7z_rename(const char *path, const char *new_path) {
#if 0
	if (*path == '\0') {
		return -ENOENT;
	}
	Node *node = data->root_node->find(path + 1);
	if (node == NULL) {
		return -ENOENT;
	}
	if (*new_path == '\0') {
		return -EINVAL;
	}
	Node *node = data->root_node->find(new_path + 1);
	if (new_node != NULL) {
		int res = get_data()->removeNode(new_node);
		if (res !=0) {
			return -res;
		}
	}

	int len = strlen(new_path);
	int oldLen = strlen(path + 1) + 1;
	char *new_name;
	if (!node->is_dir) {
		--len;
		--oldLen;
	}
	new_name = (char*)malloc(len + 1);
	if (new_path == NULL) {
		return -ENOMEM;
	}
	strcpy(new_name, new_path + 1);
	if (node->is_dir) {
		new_name[len - 1] = '/';
		new_name[len] = '\0';
	}

	try {
		struct zip *z = get_zip();
		// Renaming directory and its content recursively
		if (node->is_dir) {
			queue<Node*> q;
			q.push(node);
			while (!q.empty()) {
				Node *n = q.front();
				q.pop();
				for (nodelist_t::const_iterator i = n->childs.begin(); i != n->childs.end(); ++i) {
					Node *nn = *i;
					q.push(nn);
					char *name = (char*)malloc(len + strlen(nn->full_name) - oldLen + (nn->is_dir ? 2 : 1));
					if (name == NULL) {
						//TODO: check that we are have enough memory before entering this loop
						return -ENOMEM;
					}
					strcpy(name, new_name);
					strcpy(name + len, nn->full_name + oldLen);
					if (nn->is_dir) {
						strcat(name, "/");
					}
					zip_rename(z, nn->id, name);
					nn->rename_wo_reparenting(name);
				}
			}
		}
		zip_rename(z, node->id, new_name);
		// Must be called after loop because new_name will be truncated
		node->rename(new_name);

		return 0;
	}
	catch (...) {
		return -EIO;
	}
#endif
	return -ENOTSUP;
}

int fuse7z_utimens(const char *path, const struct timespec tv[2]) {
	Fuse7z *data = get_data();
	if (*path == '\0') {
		return -ENOENT;
	}
	Node *node = data->root_node->find(path + 1);
	if (node == NULL) {
		return -ENOENT;
	}
	node->stat.st_mtime = tv[1].tv_sec;
	return 0;
}

#if ( __FreeBSD__ >= 10 )
int fuse7z_setxattr(const char *, const char *, const char *, size_t, int, uint32_t)
#else
int fuse7z_setxattr(const char *, const char *, const char *, size_t, int)
#endif
{
	return -ENOTSUP;
}

#if ( __FreeBSD__ >= 10 )
int fuse7z_getxattr(const char *, const char *, char *, size_t, uint32_t)
#else
int fuse7z_getxattr(const char *, const char *, char *, size_t)
#endif
{
	return -ENOTSUP;
}

int fuse7z_listxattr(const char *, char *, size_t) {
	return -ENOTSUP;
}

int fuse7z_removexattr(const char *, const char *) {
	return -ENOTSUP;
}

int fuse7z_chmod(const char *, mode_t) {
	return -ENOTSUP;
}

int fuse7z_chown(const char *, uid_t, gid_t) {
	return -ENOTSUP;
}

int fuse7z_flush(const char *, struct fuse_file_info *) {
	return 0;
}

int fuse7z_fsync(const char *, int, struct fuse_file_info *) {
	return 0;
}

int fuse7z_fsyncdir(const char *, int, struct fuse_file_info *) {
	return 0;
}

int fuse7z_opendir(const char *, struct fuse_file_info *) {
	return 0;
}

int fuse7z_releasedir(const char *, struct fuse_file_info *) {
	return 0;
}

int fuse7z_access(const char *, int) {
	return 0;
}

} // extern "C"
