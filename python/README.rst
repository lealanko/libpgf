PyGF - Python bindings for libpgf
=================================

``PyGF`` is a Python binding to the libpgf_ library. The bindings are
completely ctypes-based, so there are no native-code stub libraries
involved.

.. _libpgf: http://grammaticalframework.org/libpgf/

Like the backend libraries, these bindings are still very much under
development. In particular their documentation is still severely
lacking.


Getting the code
----------------

The library is currently available only as source packages. It is
released under the `LGPL3`_.

* `PyGF-0.3.tar.gz`_

.. _LGPL3: http://www.gnu.org/licenses/lgpl.html
.. _PyGF-0.3.tar.gz: http://www.grammaticalframework.org/libpgf/PyGF-0.3.tar.gz


Prerequisites
-------------

* Python_ 3.3
* Distribute_
* Sphinx_ (optional)

.. _Python: http://python.org/
.. _Distribute: http://pythonhosted.org/distribute/
.. _Sphinx: http://sphinx-doc.org/

Installing
----------

Just the standard invocation: ``python3 setup.py install``.


Documentation
-------------

If Sphinx is available, documentation can be built with ``python3
setup.py build_sphinx``. If this page has been generated with Sphinx,
the documentation should be available at :doc:`contents`.


Credits and Copyright
---------------------

Written by Lauri Alanko <lealanko@ling.helsinki.fi>

Copyright 2013 University of Helsinki

Released under the GNU Lesser General Public License, version 3. See the
file COPYING.LESSER for the full text of the license.
