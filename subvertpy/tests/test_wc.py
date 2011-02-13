# Copyright (C) 2005-2007 Jelmer Vernooij <jelmer@samba.org>
 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 2.1 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.

# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""Subversion ra library tests."""

from StringIO import StringIO
import os

from subvertpy import (
    wc,
    )
from subvertpy.tests import (
    SubversionTestCase,
    TestCase,
    )

class VersionTest(TestCase):

    def test_version_length(self):
        self.assertEquals(4, len(wc.version()))

    def test_api_version_length(self):
        self.assertEquals(4, len(wc.api_version()))

    def test_api_version_later_same(self):
        self.assertTrue(wc.api_version() <= wc.version())


class WorkingCopyTests(TestCase):

    def test_get_adm_dir(self):
        self.assertEquals(".svn", wc.get_adm_dir())

    def test_set_adm_dir(self):
        old_dir_name = wc.get_adm_dir()
        try:
            wc.set_adm_dir("_svn")
            self.assertEquals("_svn", wc.get_adm_dir())
        finally:
            wc.set_adm_dir(old_dir_name)

    def test_is_normal_prop(self):
        self.assertTrue(wc.is_normal_prop("svn:ignore"))

    def test_is_entry_prop(self):
        self.assertTrue(wc.is_entry_prop("svn:entry:foo"))

    def test_is_wc_prop(self):
        self.assertTrue(wc.is_wc_prop("svn:wc:foo"))

    def test_match_ignore_list(self):
        if wc.api_version() < (1, 5):
            self.assertRaises(NotImplementedError, wc.match_ignore_list, "foo", [])
            return # Skip test
        self.assertTrue(wc.match_ignore_list("foo", [ "f*"]))
        self.assertTrue(wc.match_ignore_list("foo", ["foo"]))
        self.assertFalse(wc.match_ignore_list("foo", []))
        self.assertFalse(wc.match_ignore_list("foo", ["bar"]))


class AdmTests(SubversionTestCase):

    def test_has_binary_prop(self):
        repos_url = self.make_client("repos", "checkout")
        self.build_tree({"checkout/bar": "\x00\x01"})
        self.client_add('checkout/bar')
        adm = wc.WorkingCopy(None, "checkout")
        path = os.path.join(self.test_dir, "checkout/bar")
        self.assertFalse(adm.has_binary_prop(path))
        adm.close()

    def test_get_ancestry(self):
        repos_url = self.make_client("repos", "checkout")
        self.build_tree({"checkout/bar": "\x00\x01"})
        self.client_add('checkout/bar')
        adm = wc.WorkingCopy(None, "checkout")
        path = os.path.join(self.test_dir, "checkout/bar")
        self.assertEquals(("%s/bar" % repos_url, 0), adm.get_ancestry("checkout/bar"))
        adm.close()

    def test_maybe_set_repos_root(self):
        repos_url = self.make_client("repos", "checkout")
        adm = wc.WorkingCopy(None, "checkout")
        adm.maybe_set_repos_root(os.path.join(self.test_dir, "checkout"), repos_url)
        adm.close()

    def test_add_repos_file(self):
        repos_url = self.make_client("repos", "checkout")
        adm = wc.WorkingCopy(None, "checkout", True)
        adm.add_repos_file("checkout/bar", StringIO("oldbasecontents"), StringIO("oldcontents"), {}, {})
        adm.add_repos_file("checkout/bar", StringIO("basecontents"), StringIO("contents"), {}, {})
        self.assertEquals("basecontents", wc.get_pristine_contents("checkout/bar").read())

    def test_mark_missing_deleted(self):
        repos_url = self.make_client("repos", "checkout")
        self.build_tree({"checkout/bar": "\x00\x01"})
        self.client_add('checkout/bar')
        adm = wc.WorkingCopy(None, "checkout", True)
        os.remove("checkout/bar")
        adm.mark_missing_deleted("checkout/bar")
        self.assertFalse(os.path.exists("checkout/bar"))

    def test_remove_from_revision_control(self):
        repos_url = self.make_client("repos", "checkout")
        self.build_tree({"checkout/bar": "\x00\x01"})
        self.client_add('checkout/bar')
        adm = wc.WorkingCopy(None, "checkout", True)
        adm.remove_from_revision_control("bar")
        self.assertTrue(os.path.exists("checkout/bar"))

    def test_relocate(self):
        repos_url = self.make_client("repos", "checkout")
        adm = wc.WorkingCopy(None, "checkout", True)
        adm.relocate("checkout", "file://", "http://")
