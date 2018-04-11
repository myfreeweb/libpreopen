
/*
 * Copyright (c) 2016 Stanley Uche Godfrey
 * Copyright (c) 2016, 2018 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed at Memorial University under the
 * NSERC Discovery program (RGPIN-2015-06048).
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef LIBPO_H
#define LIBPO_H

#include <sys/capsicum.h>



#include <sys/cdefs.h>

#include <stdbool.h>


__BEGIN_DECLS

/**
 * A mapping from paths to pre-opened directories.
 *
 * This type is opaque to clients, but it is reference-counted and can be
 * thought of as containing a set (with no particular ordering guarantees)
 * of path->dirfd mappings.
 */
struct po_map;

/**
 * A callback that can be used to inpect the contents of a @ref po_map.
 *
 * This callback can be invoked by iteration code to expose elements of a
 * @ref po_map (in no particular order).
 *
 * @returns   whether or not to continue iterating over the @ref po_map
 */
typedef bool (po_dir_callback)(const char *dirname, int dirfd, cap_rights_t);

/**
 * A simple @ref po_dir_callback that will print a @ref po_map's entries,
 * one per line.
 */
po_dir_callback	po_dir_print;

/**
 * A filesystem path, relative to a directory descriptor.
 */
struct po_relpath {
	/** The directory the path is relative to */
	int dirfd;

	/** The path, relative to the directory represented by @ref dirfd */
	const char *relative_path;
};

/**
 * Create a @ref po_map of at least the specified capacity.
 *
 * The returned @ref po_map will have a reference count of 1.
 */
struct po_map* po_map_create(int capacity);

/**
 * Release a reference to a @ref po_map.
 *
 * This may cause memory to be freed.
 */
void po_map_release(struct po_map *);

/**
 * Iterate over a @ref po_map, invoking a callback for each element in the map.
 *
 * This function will cause @b callback to be invoked repeatedly, in no
 * particular order, until the entire map has been iterated over or until the
 * callback returns `false`.
 *
 * @return   number of elements iterated over
 */
size_t po_map_foreach(const struct po_map*, po_dir_callback);

/**
 * Retrieve (and possibly create) the default map.
 *
 * This can fail if there is no existing map and memory allocation fails.
 */
struct po_map* po_map_get(void);

/**
 * Set the default map, taking ownership of its memory allocation(s).
 *
 * If there is an existing default map, it will be freed before it is replaced.
 * It is permissible to pass in a NULL map in order to clear the current
 * default map.
 */
void po_map_set(struct po_map*);

/**
 * Add an already-opened directory to a @ref po_map.
 *
 * @param   map     the map to add the path->fd mapping to
 * @param   path    the path that will map to this directory
 *                  (which may or may not be the path used to open it)
 * @param   fd      the directory descriptor (must be a directory!)
 */
struct po_map* po_add(struct po_map *map, const char *path, int fd);

/**
 * Pre-open a path and store it in a @ref po_map for later use.
 *
 * @returns the file descriptor of the opened directory or -1 if
 *          @b path is not a directory or cannot be opened or if
 *          the @ref po_map cannot store the directory (e.g., resizing fails)
 */
int po_preopen(struct po_map *, const char *path);

/**
 * 
 * 
 *Splits file name from absolute file a path and returns the absolute
 * path to the directory containing the file.
 * @param   relative_path  the absolute path to the file
 */

char* po_split_file_fromPath(const char *relative_path);

/**
 * Find a directory whose path is a prefix of @b path and (on platforms that
 * support Capsicum) that has the rights required by @b rights.
 *
 * @param   map     the map to look for a directory in
 * @param   path    the path we want to find a pre-opened prefix for
 * @param   rights  if non-NULL on a platform with Capsicum support,
 *                  the rights any directory descriptor must have to
 *                  qualify as a match
 * @returns a @ref po_relpath containing the descriptor of the best-match
 *          directory in the map (or -1 if none was found) and the remaining
 *          path, relative to the file (or undefined if no match found)
 */
struct po_relpath po_find(struct po_map *map, const char *path,
	cap_rights_t *rights);

/**
 * Retrieve a message from with the last libpreopen error.
 *
 * @returns NULL if there are no errors, null-terminated string otherwise
 */
const char* po_last_error(void);

/**
 * Pack a `struct po_map` into a shared memory segment.
 *
 * To inherit a `po_map` across the process execution boundary, it needs to be
 * packed into an inheritable form such as a shared memory segment. This can
 * then be unpacked in the child process for direct access.
 *
 * @param map     the map to pack into shared memory
 *
 * @returns       a file descriptor of a shared memory segment
 *                (or -1 on error)
 */
int po_pack(struct po_map *map);

/**
 * Unpack a `struct po_map` from a file.
 *
 * Using the representation generated by `po_pack`, unpack a `po_map`
 * and make it available for normal usage.
 *
 * @param fd      a file containing a packed `po_map` representation
 */
struct po_map* po_unpack(int fd);

__END_DECLS

#endif /* !LIBPO_H */

