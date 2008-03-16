# Copyright (C) 2008 Jelmer Vernooij <jelmer@samba.org>

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

from apr cimport apr_pool_t, apr_initialize, apr_hash_t, apr_pool_destroy, apr_time_t, apr_hash_first, apr_hash_next, apr_hash_this, apr_hash_index_t, apr_array_header_t
from types cimport svn_error_t, svn_version_t, svn_boolean_t, svn_cancel_func_t , svn_string_t, svn_string_ncreate, svn_node_kind_t, svn_revnum_t, svn_prop_t

from core cimport check_error, Pool, py_cancel_func

apr_initialize()

cdef extern from "Python.h":
    object PyString_FromStringAndSize(char *, long len)


cdef extern from "svn_wc.h":
    ctypedef struct svn_wc_adm_access_t
    ctypedef enum svn_wc_schedule_t:
        svn_wc_schedule_normal
        svn_wc_schedule_add
        svn_wc_schedule_delete
        svn_wc_schedule_replace

    ctypedef struct svn_wc_entry_t:
        char *name
        svn_revnum_t revision
        char *url
        char *repos
        char *uuid
        svn_node_kind_t kind
        svn_wc_schedule_t schedule
        svn_boolean_t copied
        svn_boolean_t deleted
        svn_boolean_t absent
        svn_boolean_t incomplete
        char *copyfrom_url
        svn_revnum_t copyfrom_rev
        char *conflict_old
        char *conflict_new
        char *conflict_wrk
        char *prejfile
        apr_time_t text_time
        apr_time_t prop_time
        char *checksum
        svn_revnum_t cmt_rev
        apr_time_t cmt_date
        char *cmt_author
        char *lock_token
        char *lock_owner
        char *lock_comment
        apr_time_t lock_creation_date
        svn_boolean_t has_props
        svn_boolean_t has_prop_mods
        char *cachable_props
        char *present_props


    svn_version_t *svn_wc_version()
    svn_error_t *svn_wc_adm_open3(svn_wc_adm_access_t **adm_access,
                                  svn_wc_adm_access_t *associated,
                                  char *path,
                                  svn_boolean_t write_lock,
                                  int depth,
                                  svn_cancel_func_t cancel_func,
                                  cancel_baton,
                                  apr_pool_t *pool)
    svn_error_t *svn_wc_adm_close(svn_wc_adm_access_t *adm_access)
    char *svn_wc_adm_access_path(svn_wc_adm_access_t *adm_access)
    svn_boolean_t svn_wc_adm_locked(svn_wc_adm_access_t *adm_access)
    svn_error_t *svn_wc_locked(svn_boolean_t *locked, char *path, apr_pool_t *pool)
    ctypedef struct svn_wc_revision_status_t:
        long min_rev
        long max_rev
        int switched
        int modified
    svn_error_t *svn_wc_revision_status(svn_wc_revision_status_t **result_p,
                       char *wc_path,
                       char *trail_url,
                       svn_boolean_t committed,
                       svn_cancel_func_t cancel_func,
                       cancel_baton,
                       apr_pool_t *pool)
    svn_error_t *svn_wc_prop_get(svn_string_t **value,
                             char *name,
                             char *path,
                             svn_wc_adm_access_t *adm_access,
                             apr_pool_t *pool)
    svn_error_t *svn_wc_entries_read(apr_hash_t **entries,
                                 svn_wc_adm_access_t *adm_access,
                                 svn_boolean_t show_hidden,
                                 apr_pool_t *pool)
    svn_error_t *svn_wc_prop_set2(char *name,
                              svn_string_t *value,
                              char *path,
                              svn_wc_adm_access_t *adm_access,
                              svn_boolean_t skip_checks,
                              apr_pool_t *pool)
    svn_error_t *svn_wc_entry(svn_wc_entry_t **entry,
                          char *path,
                          svn_wc_adm_access_t *adm_access,
                          svn_boolean_t show_hidden,
                          apr_pool_t *pool)

    svn_boolean_t svn_wc_is_normal_prop(char *name)
    svn_boolean_t svn_wc_is_wc_prop(char *name)
    svn_boolean_t svn_wc_is_entry_prop(char *name)

    svn_error_t *svn_wc_get_prop_diffs(apr_array_header_t **propchanges,
                                   apr_hash_t **original_props,
                                   char *path,
                                   svn_wc_adm_access_t *adm_access,
                                   apr_pool_t *pool)
    svn_error_t *svn_wc_get_pristine_copy_path(char *path,
                                           char **pristine_path,
                                           apr_pool_t *pool)

    char *svn_wc_get_adm_dir(apr_pool_t *pool)

def version():
    """Get libsvn_wc version information.

    :return: tuple with major, minor, patch version number and tag.
    """
    return (svn_wc_version().major, svn_wc_version().minor, 
            svn_wc_version().minor, svn_wc_version().tag)

class Entry:
    def __init__(self, name, revision, url, repos, uuid, kind, schedule, copied=False, deleted=False, absent=False, incomplete=False):
        self.name = name
        self.revision = revision
        self.url = url
        self.uuid = uuid
        self.repos = repos
        self.kind = kind
        self.schedule = schedule
        self.copied = copied
        self.deleted = deleted
        self.absent = absent
        self.incomplete = incomplete

cdef py_entry(svn_wc_entry_t *entry):
    if entry.uuid == NULL:
        uuid = None
    else:
        uuid = entry.uuid
    if entry.url == NULL:
        url = None
    else:
        url = entry.url
    if entry.repos == NULL:
        repos = None
    else:
        repos = entry.repos
    ret = Entry(entry.name, entry.revision, url, repos, uuid, entry.kind, entry.schedule, entry.copied, entry.deleted, entry.absent, entry.incomplete)
    ret.cmt_rev = entry.cmt_rev
    # FIXME: entry.copyfrom_url, entry.copyfrom_rev, entry.conflict_old, entry.conflict_new, entry.conflict_wrk, entry.prejfile, entry.text_time, entry.prop_time, entry.checksum, entry.cmt_date, entry.cmt_author, entry.lock_token, entry.lock_owner, entry.lock_comment, entry.lock_creation_date, entry.has_props, entry.has_prop_mods, entry.cachable_props, entry.present_props)
    return ret

cdef class WorkingCopy:
    cdef svn_wc_adm_access_t *adm
    cdef apr_pool_t *pool
    def __init__(self, WorkingCopy associated, path, write_lock=False, depth=0, 
                 cancel_func=None):
        cdef svn_wc_adm_access_t *parent_wc
        self.pool = Pool(NULL)
        if associated is None:
            parent_wc = NULL
        else:
            parent_wc = associated.adm
        check_error(svn_wc_adm_open3(&self.adm, parent_wc, path, 
                     write_lock, depth, py_cancel_func, cancel_func, 
                     self.pool))

    def access_path(self):
        return svn_wc_adm_access_path(self.adm)

    def locked(self):
        return svn_wc_adm_locked(self.adm)

    def prop_get(self, name, path):
        cdef svn_string_t *value
        cdef apr_pool_t *temp_pool
        temp_pool = Pool(self.pool)
        check_error(svn_wc_prop_get(&value, name, path, self.adm, temp_pool))
        ret = PyString_FromStringAndSize(value.data, value.len)
        apr_pool_destroy(temp_pool)
        return ret

    def prop_set(self, name, value, path, skip_checks=False):
        cdef apr_pool_t *temp_pool
        cdef svn_string_t *cvalue
        temp_pool = Pool(self.pool)
        cvalue = svn_string_ncreate(value, len(value), temp_pool)
        check_error(svn_wc_prop_set2(name, cvalue, path, self.adm, 
                    skip_checks, temp_pool))
        apr_pool_destroy(temp_pool)

    def entries_read(self, show_hidden=False):
        cdef apr_hash_t *entries
        cdef apr_pool_t *temp_pool
        cdef apr_hash_index_t *idx
        cdef char *key
        cdef long klen
        cdef svn_wc_entry_t *entry
        temp_pool = Pool(self.pool)
        check_error(svn_wc_entries_read(&entries, self.adm, 
                     show_hidden, temp_pool))
        py_entries = {}
        idx = apr_hash_first(temp_pool, entries)
        while idx:
            apr_hash_this(idx, <void **>&key, &klen, <void **>&entry)
            py_entries[key] = py_entry(entry)
            idx = apr_hash_next(idx)
        apr_pool_destroy(temp_pool)
        return py_entries

    def entry(self, path, show_hidden=False):
        cdef apr_pool_t *temp_pool
        cdef svn_wc_entry_t *entry
        temp_pool = Pool(self.pool)
        check_error(svn_wc_entry(&entry, path, self.adm, show_hidden, temp_pool))
        apr_pool_destroy(temp_pool)

        return py_entry(entry)

    def get_prop_diffs(self, path):
        cdef apr_pool_t *temp_pool
        cdef apr_array_header_t *propchanges
        cdef apr_hash_t *original_props
        cdef apr_hash_index_t *idx
        cdef svn_string_t *string
        cdef char *key
        cdef long klen
        cdef svn_prop_t *el
        temp_pool = Pool(self.pool)
        check_error(svn_wc_get_prop_diffs(&propchanges, &original_props, 
                    path, self.adm, temp_pool))
        py_propchanges = []
        for i in range(propchanges.nelts):
            el = <svn_prop_t *>propchanges.elts[i]
            py_propchanges.append((el.name, PyString_FromStringAndSize(el.value.data, el.value.len)))
        py_orig_props = {}
        idx = apr_hash_first(temp_pool, original_props)
        while idx:
            apr_hash_this(idx, <void **>&key, &klen, <void **>&string)
            py_orig_props[key] = PyString_FromStringAndSize(string.data, string.len)
            idx = apr_hash_next(idx)
        apr_pool_destroy(temp_pool)
        return (py_propchanges, py_orig_props)

    def close(self):
        if self.adm != NULL:
            svn_wc_adm_close(self.adm)
            self.adm = NULL

    def __dealloc__(self):
        apr_pool_destroy(self.pool)


def revision_status(wc_path, trail_url=None, committed=False, cancel_func=None):
    """Determine the revision status of a specified working copy.

    :return: Tuple with minimum and maximum revnums found, whether the 
             working copy was switched and whether it was modified.
    """
    cdef svn_wc_revision_status_t *revstatus
    cdef apr_pool_t *temp_pool
    cdef char *c_trail_url
    temp_pool = Pool(NULL)
    if trail_url is None:
        c_trail_url = NULL
    else:
        c_trail_url = trail_url
    check_error(svn_wc_revision_status(&revstatus, wc_path, c_trail_url,
                 committed, py_cancel_func, cancel_func, temp_pool))
    ret = (revstatus.min_rev, revstatus.max_rev, 
            revstatus.switched, revstatus.modified)
    apr_pool_destroy(temp_pool)
    return ret

def is_normal_prop(name):
    return svn_wc_is_normal_prop(name)

def is_wc_prop(name):
    return svn_wc_is_wc_prop(name)

def is_entry_prop(name):
    return svn_wc_is_entry_prop(name)

def get_adm_dir():
    cdef apr_pool_t *pool
    pool = Pool(NULL)
    ret = svn_wc_get_adm_dir(pool)
    apr_pool_destroy(pool)
    return ret

def get_pristine_copy_path(path):
    cdef apr_pool_t *pool
    cdef char *pristine_path
    pool = Pool(NULL)
    try:
        check_error(svn_wc_get_pristine_copy_path(path, &pristine_path, pool))
        ret = pristine_path
    finally:
        apr_pool_destroy(pool)
    return ret

SCHEDULE_NORMAL = 0
SCHEDULE_ADD = 1
SCHEDULE_DELETE = 2
SCHEDULE_REPLACE = 3
