/* diritor.h: Iterator through entries in a directory.
 *
 * Copyright (C) 2007,2008,2010 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef OMEGA_INCLUDED_DIRITOR_H
#define OMEGA_INCLUDED_DIRITOR_H

#include <string>

#include "safedirent.h"
#include "safeerrno.h"
#include "safesysstat.h"

#include <sys/types.h>
#include <grp.h> // For getgrgid().
#include <pwd.h> // For getpwuid().

#include "common/noreturn.h"

class DirectoryIterator {
    std::string path;
    DIR * dir;
    struct dirent *entry;
    struct stat statbuf;
    bool statbuf_valid;
    bool follow_symlinks;

    void call_stat();

    void ensure_statbuf_valid() {
	if (!statbuf_valid) {
	    call_stat();
	    statbuf_valid = true;
	}
    }

  public:

    DirectoryIterator(bool follow_symlinks_)
	: dir(NULL), follow_symlinks(follow_symlinks_) { }

    ~DirectoryIterator() {
	if (dir) closedir(dir);
    }

    /// Start iterating through entries in @a path.
    //
    //  Throws a std::string exception upon failure.
    void start(const std::string & path);

    /// Read the next directory entry which doesn't start with ".".
    //
    //  We do this to skip ".", "..", and Unix hidden files.
    //
    //  @return false if there are no more entries.
    bool next() {
	errno = 0;
	do {
	    entry = readdir(dir);
	} while (entry && entry->d_name[0] == '.');
	statbuf_valid = false;
	if (entry == NULL && errno != 0) next_failed();
	return (entry != NULL);
    }

    XAPIAN_NORETURN(void next_failed() const);

    const char * leafname() const { return entry->d_name; }

    typedef enum { REGULAR_FILE, DIRECTORY, OTHER } type;

    type get_type() {
#ifdef DT_UNKNOWN
	/* Possible values:
	 * DT_UNKNOWN DT_FIFO DT_CHR DT_DIR DT_BLK DT_REG DT_LNK DT_SOCK DT_WHT
	 */
	switch (entry->d_type) {
	    case DT_UNKNOWN:
		// The current filing system doesn't support d_type.
		break;
	    case DT_REG:
		return REGULAR_FILE;
	    case DT_DIR:
		return DIRECTORY;
#ifdef HAVE_LSTAT
	    case DT_LNK:
		if (follow_symlinks) break;
		return OTHER;
#endif
	    default:
		return OTHER;
	}
#endif

	ensure_statbuf_valid();

	if (S_ISREG(statbuf.st_mode)) return REGULAR_FILE;
	if (S_ISDIR(statbuf.st_mode)) return DIRECTORY;
	return OTHER;
    }

    off_t get_size() {
	ensure_statbuf_valid();
	return statbuf.st_size;
    }

    off_t get_mtime() {
	ensure_statbuf_valid();
	return statbuf.st_mtime;
    }

    const char * get_owner() {
	ensure_statbuf_valid();
	struct passwd * pwentry = getpwuid(statbuf.st_uid);
	return pwentry ? pwentry->pw_name : NULL;
    }

    const char * get_group() {
	ensure_statbuf_valid();
	struct group * grentry = getgrgid(statbuf.st_gid);
	return grentry ? grentry->gr_name : NULL;
    }

    bool is_owner_readable() {
	ensure_statbuf_valid();
	return (statbuf.st_mode & S_IRUSR);
    }

    bool is_group_readable() {
	ensure_statbuf_valid();
	return (statbuf.st_mode & S_IRGRP);
    }

    bool is_other_readable() {
	ensure_statbuf_valid();
	return (statbuf.st_mode & S_IROTH);
    }
};

#endif // OMEGA_INCLUDED_DIRITOR_H