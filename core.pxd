# Copyright (C) 2008 Jelmer Vernooij <jelmer@samba.org>
# vim: ft=pyrex

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

from apr cimport apr_pool_t, apr_array_header_t, apr_hash_t
from types cimport svn_error_t, svn_lock_t, svn_stream_t
cdef apr_pool_t *Pool(apr_pool_t *parent)
cdef check_error(svn_error_t *error)
cdef svn_error_t *py_cancel_func(cancel_baton)
cdef wrap_lock(svn_lock_t *)
cdef apr_array_header_t *string_list_to_apr_array(apr_pool_t *pool, object l)
cdef svn_error_t *py_svn_log_wrapper(baton, apr_hash_t *changed_paths, long revision, char *author, char *date, char *message, apr_pool_t *pool) except *
cdef svn_stream_t *new_py_stream(apr_pool_t *pool, object py)
cdef svn_stream_t *string_stream(apr_pool_t *pool, text)
cdef prop_hash_to_dict(apr_hash_t *)
