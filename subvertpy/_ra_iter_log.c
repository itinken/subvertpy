/*
 * Copyright © 2010 Jelmer Vernooij <jelmer@samba.org>
 * -*- coding: utf-8 -*-
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <pythread.h>

struct log_entry {
	PyObject *tuple;
	struct log_entry *next;
};

typedef struct {
	PyObject_HEAD
	svn_revnum_t start, end;
	svn_boolean_t discover_changed_paths;
	svn_boolean_t strict_node_history;
	svn_boolean_t include_merged_revisions;
	int limit;
	apr_pool_t *pool;
	apr_array_header_t *apr_paths;
	apr_array_header_t *apr_revprops;
	RemoteAccessObject *ra;
	svn_boolean_t done;
	PyObject *exception;
	int queue_size;
	struct log_entry *head;
	struct log_entry *tail;
} LogIteratorObject;

static void log_iter_dealloc(PyObject *self)
{
	LogIteratorObject *iter = (LogIteratorObject *)self;

	while (iter->head) {
		struct log_entry *e = iter->head;
		Py_DECREF(e->tuple);
		iter->head = e->next;
		free(e);
	}
	Py_DECREF(iter->ra);
	apr_pool_destroy(iter->pool);
}

static PyObject *log_iter_next(LogIteratorObject *iter)
{
	struct log_entry *first;
	PyObject *ret;

	while (iter->head == NULL) {
		/* Done, raise stopexception */
		if (iter->done) {
			if (iter->exception != NULL) {
				PyObject *exccls = (PyObject *)PyErr_GetSubversionExceptionTypeObject();
				if (exccls == NULL)
					return NULL;
				PyErr_SetObject(exccls, iter->exception);
			} else {
				PyErr_SetNone(PyExc_StopIteration);
			}
			return NULL;
		} else {
			Py_BEGIN_ALLOW_THREADS
			/* FIXME: Don't waste cycles */
			Py_END_ALLOW_THREADS
		}
	}
	first = iter->head;
	ret = iter->head->tuple;
	iter->head = first->next;
	if (first == iter->tail)
		iter->tail = NULL;
	free(first);
	iter->queue_size--;
	return ret;
}

static PyObject *py_iter_append(LogIteratorObject *iter, PyObject *tuple)
{
	struct log_entry *entry;

	entry = calloc(sizeof(struct log_entry), 1);
	if (entry == NULL) {
		PyErr_NoMemory();
		return NULL;
	}

	entry->tuple = tuple;
	if (iter->tail == NULL) {
		iter->tail = entry;
	} else {
		iter->tail->next = entry;
		iter->tail = entry;
	}
	if (iter->head == NULL)
		iter->head = entry;

	iter->queue_size++;

	Py_RETURN_NONE;
}

PyTypeObject LogIterator_Type = {
	PyObject_HEAD_INIT(NULL) 0,
	"_ra.LogIterator", /*	const char *tp_name;  For printing, in format "<module>.<name>" */
	sizeof(LogIteratorObject), 
	0,/*	Py_ssize_t tp_basicsize, tp_itemsize;  For allocation */
	
	/* Methods to implement standard operations */
	
	(destructor)log_iter_dealloc, /*	destructor tp_dealloc;	*/
	NULL, /*	printfunc tp_print;	*/
	NULL, /*	getattrfunc tp_getattr;	*/
	NULL, /*	setattrfunc tp_setattr;	*/
	NULL, /*	cmpfunc tp_compare;	*/
	NULL, /*	reprfunc tp_repr;	*/
	
	/* Method suites for standard classes */
	
	NULL, /*	PyNumberMethods *tp_as_number;	*/
	NULL, /*	PySequenceMethods *tp_as_sequence;	*/
	NULL, /*	PyMappingMethods *tp_as_mapping;	*/
	
	/* More standard operations (here for binary compatibility) */
	
	NULL, /*	hashfunc tp_hash;	*/
	NULL, /*	ternaryfunc tp_call;	*/
	NULL, /*	reprfunc tp_str;	*/
	NULL, /*	getattrofunc tp_getattro;	*/
	NULL, /*	setattrofunc tp_setattro;	*/
	
	/* Functions to access object as input/output buffer */
	NULL, /*	PyBufferProcs *tp_as_buffer;	*/
	
	/* Flags to define presence of optional/expanded features */
	Py_TPFLAGS_HAVE_ITER, /*	long tp_flags;	*/
	
	NULL, /*	const char *tp_doc;  Documentation string */
	
	/* Assigned meaning in release 2.0 */
	/* call function for all accessible objects */
	NULL, /*	traverseproc tp_traverse;	*/
	
	/* delete references to contained objects */
	NULL, /*	inquiry tp_clear;	*/
	
	/* Assigned meaning in release 2.1 */
	/* rich comparisons */
	NULL, /*	richcmpfunc tp_richcompare;	*/
	
	/* weak reference enabler */
	0, /*	Py_ssize_t tp_weaklistoffset;	*/
	
	/* Added in release 2.2 */
	/* Iterators */
	PyObject_SelfIter, /*	getiterfunc tp_iter;	*/
	(iternextfunc)log_iter_next, /*	iternextfunc tp_iternext;	*/
};

#if SVN_VER_MAJOR == 1 && SVN_VER_MINOR >= 5
static svn_error_t *py_iter_log_entry_cb(void *baton, svn_log_entry_t *log_entry, apr_pool_t *pool)
{
	PyObject *revprops, *py_changed_paths, *ret, *tuple;
	LogIteratorObject *iter = (LogIteratorObject *)baton;

	PyGILState_STATE state;

	state = PyGILState_Ensure();

	py_changed_paths = pyify_changed_paths(log_entry->changed_paths, pool);
	if (py_changed_paths == NULL) {
		PyGILState_Release(state);
		return py_svn_error();
	}

	revprops = prop_hash_to_dict(log_entry->revprops);
	if (revprops == NULL) {
		Py_DECREF(py_changed_paths);
		PyGILState_Release(state);
		return py_svn_error();
	}

	tuple = Py_BuildValue("NlNb", py_changed_paths,
						log_entry->revision, revprops, log_entry->has_children);
	if (tuple == NULL) {
		Py_DECREF(revprops);
		Py_DECREF(py_changed_paths);
		PyGILState_Release(state);
		return py_svn_error();
	}

	ret = py_iter_append(iter, tuple);
	if (ret == NULL) {
		Py_DECREF(tuple);
		PyGILState_Release(state);
		return py_svn_error();
	}

	Py_DECREF(ret);

	PyGILState_Release(state);

	return NULL;
}
#else
static svn_error_t *py_iter_log_cb(void *baton, apr_hash_t *changed_paths, svn_revnum_t revision, const char *author, const char *date, const char *message, apr_pool_t *pool)
{
	PyObject *revprops, *py_changed_paths, *ret, *obj, *tuple;
	LogIteratorObject *iter = (LogIteratorObject *)baton;

	PyGILState_STATE state;

	state = PyGILState_Ensure();

	py_changed_paths = pyify_changed_paths(changed_paths, pool);
	if (py_changed_paths == NULL) {
		PyGILState_Release(state);
		return py_svn_error();
	}

	revprops = PyDict_New();
	if (revprops == NULL) {
		Py_DECREF(py_changed_paths);
		PyGILState_Release(state);
		return py_svn_error();
	}

	if (message != NULL) {
		obj = PyString_FromString(message);
		PyDict_SetItemString(revprops, SVN_PROP_REVISION_LOG, obj);
		Py_DECREF(obj);
	}
	if (author != NULL) {
		obj = PyString_FromString(author);
		PyDict_SetItemString(revprops, SVN_PROP_REVISION_AUTHOR, obj);
		Py_DECREF(obj);
	}
	if (date != NULL) {
		obj = PyString_FromString(date);
		PyDict_SetItemString(revprops, SVN_PROP_REVISION_DATE, 
							 obj);
		Py_DECREF(obj);
	}
	tuple = Py_BuildValue("NlN", py_changed_paths, revision, revprops);
	if (tuple == NULL) {
		Py_DECREF(py_changed_paths);
		Py_DECREF(revprops);
		PyGILState_Release(state);
		return py_svn_error();
	}

	ret = py_iter_append(iter, tuple);

	if (ret == NULL) {
		Py_DECREF(tuple);
		PyGILState_Release(state);
		return py_svn_error();
	}

	Py_DECREF(ret);

	PyGILState_Release(state);

	return NULL;
}
#endif


static void py_iter_log(void *baton)
{
	LogIteratorObject *iter = (LogIteratorObject *)baton;
	svn_error_t *error;
	PyGILState_STATE state;

#if SVN_VER_MAJOR == 1 && SVN_VER_MINOR >= 5
	error = svn_ra_get_log2(iter->ra->ra, 
			iter->apr_paths, iter->start, iter->end, iter->limit,
			iter->discover_changed_paths, iter->strict_node_history, 
			iter->include_merged_revisions, iter->apr_revprops,
			py_iter_log_entry_cb, iter, iter->pool);
#else
	error = svn_ra_get_log(iter->ra->ra, 
			iter->apr_paths, iter->start, iter->end, iter->limit,
			iter->discover_changed_paths, iter->strict_node_history, py_iter_log_cb, 
			iter, iter->pool);
#endif
	state = PyGILState_Ensure();
	iter->done = TRUE;
	iter->ra->busy = false;
	if (error != NULL) {
		iter->exception = PyErr_NewSubversionException(error);
	}
	Py_DECREF(iter);
	PyGILState_Release(state);
}

PyObject *ra_iter_log(PyObject *self, PyObject *args, PyObject *kwargs)
{
	char *kwnames[] = { "paths", "start", "end", "limit",
		"discover_changed_paths", "strict_node_history", "include_merged_revisions", "revprops", NULL };
	PyObject *paths;
	svn_revnum_t start = 0, end = 0;
	int limit=0; 
	bool discover_changed_paths=false, strict_node_history=true,include_merged_revisions=false;
	RemoteAccessObject *ra = (RemoteAccessObject *)self;
	PyObject *revprops = Py_None;
	LogIteratorObject *ret;
	apr_pool_t *temp_pool;
	apr_array_header_t *apr_paths;
	apr_array_header_t *apr_revprops;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Oll|ibbbO:iter_log", kwnames, 
						 &paths, &start, &end, &limit,
						 &discover_changed_paths, &strict_node_history,
						 &include_merged_revisions, &revprops))
		return NULL;

	if (ra_check_busy(ra))
		return NULL;

	temp_pool = Pool(NULL);
	if (temp_pool == NULL)
		return NULL;
	if (paths == Py_None) {
		/* The subversion libraries don't behave as expected, 
		 * so tweak our own parameters a bit. */
		apr_paths = apr_array_make(temp_pool, 1, sizeof(char *));
		APR_ARRAY_PUSH(apr_paths, char *) = apr_pstrdup(temp_pool, "");
	} else if (!path_list_to_apr_array(temp_pool, paths, &apr_paths)) {
		apr_pool_destroy(temp_pool);
		return NULL;
	}

#if SVN_VER_MAJOR <= 1 && SVN_VER_MINOR < 5
	if (revprops == Py_None) {
		PyErr_SetString(PyExc_NotImplementedError, "fetching all revision properties not supported");	
		apr_pool_destroy(temp_pool);
		return NULL;
	} else if (!PySequence_Check(revprops)) {
		PyErr_SetString(PyExc_TypeError, "revprops should be a sequence");
		apr_pool_destroy(temp_pool);
		return NULL;
	} else {
		int i;
		for (i = 0; i < PySequence_Size(revprops); i++) {
			const char *n = PyString_AsString(PySequence_GetItem(revprops, i));
			if (strcmp(SVN_PROP_REVISION_LOG, n) && 
				strcmp(SVN_PROP_REVISION_AUTHOR, n) &&
				strcmp(SVN_PROP_REVISION_DATE, n)) {
				PyErr_SetString(PyExc_NotImplementedError, 
								"fetching custom revision properties not supported");	
				apr_pool_destroy(temp_pool);
				return NULL;
			}
		}
	}

	if (include_merged_revisions) {
		PyErr_SetString(PyExc_NotImplementedError, 
			"include_merged_revisions not supported in Subversion 1.4");
		apr_pool_destroy(temp_pool);
		return NULL;
	}
#endif

	if (!string_list_to_apr_array(temp_pool, revprops, &apr_revprops)) {
		apr_pool_destroy(temp_pool);
		return NULL;
	}

	ret = PyObject_New(LogIteratorObject, &LogIterator_Type);
	ret->ra = ra;
	Py_INCREF(ret->ra);
	ret->start = start;
	ret->exception = NULL;
	ret->discover_changed_paths = discover_changed_paths;
	ret->end = end;
	ret->limit = limit;
	ret->apr_paths = apr_paths;
	ret->pool = temp_pool;
	ret->include_merged_revisions = include_merged_revisions;
	ret->apr_revprops = apr_revprops;
	ret->done = FALSE;
	ret->queue_size = 0;
	ret->head = NULL;
	ret->tail = NULL;

	Py_INCREF(ret);
	PyThread_start_new_thread(py_iter_log, ret);

	return (PyObject *)ret;
}

