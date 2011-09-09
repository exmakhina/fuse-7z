#ifndef FUSE_7Z_NODE_H
#define FUSE_7Z_NODE_H

#include <string>
#include <vector>
#include <list>
#include <sys/stat.h>
#include <map>
#include <cstring>

class Node;

class NodeBuffer {
	public:
	NodeBuffer() {}
	virtual ~NodeBuffer() {}
};

struct ltstr {
    bool operator() (const char* s1, const char* s2) const {
        return strcmp(s1, s2) < 0;
    }
};

typedef std::map <const char *, Node*, ltstr> nodelist_t;

class Node {
	private:
	static int max_inode;

	enum nodeState {
		CLOSED,
		OPENED,
		CHANGED,
		NEW
	};

	int open_count;
	nodeState state;

	public:
	NodeBuffer *buffer;

	char const *name;
	std::string sname;
	bool is_dir;
	int id;
	nodelist_t childs;
	Node *parent;
	struct stat stat;

	std::string fullname() const;

	void parse_name(char const *fname);
	void attach();

	public:
	static const int ROOT_NODE_INDEX, NEW_NODE_INDEX;

	Node(Node * parent, char const * name);
	~Node();


	Node * find(char const *);
	Node * insert(char * leaf);

#if 0
	void detach();
	void rename(char *fname);

	void rename_wo_reparenting(char *new_name);
	int open();
	int read(char *buf, size_t size, offset_t offset) const;
	int write(const char *buf, size_t size, offset_t offset);
	int close();
	int save();
	int truncate(offset_t offset);

#if defined(FUSE7Z_WRITE)
	inline bool isChanged() const {
		return state == CHANGED || state == NEW;
	}
#endif

	offset_t size() const;
#endif
};

#endif // FUSE_7Z_NODE_H

