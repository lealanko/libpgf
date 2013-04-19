PyGF documentation
==================

This is the documentation for ``pygf``, a Python binding for the libpgf_ library. Please note that this package is still in early development and lacks a number of features.

.. _libpgf: http://grammaticalframework.org/libpgf/


Contents:

.. toctree::
   :maxdepth: 2

Tutorial
========

The module :py:mod:`pgf.pygf` provides a simplified interface that attempts to mimic the functionality of the `GF interpreter`_. Here is a sample session::

    >>> from pgf.pygf import *
    >>> import_pgf('Phrasebook.pgf')

This loads a PGF file into the system and makes its grammars available. We can use the :py:func:`pgf.pygf.parse` function to parse sentences of the grammar::

     >>> parse('where is the museum', lang='en_US')
     <Result:
     0: PQuestion (WherePlace (ThePlace Museum))
     >

The returned value is a :py:class:`pgf.pygf.Result` object, a special kind of iterable which in this case lists parse trees of type :py:class:`pgf.Expr`. The result object supports a convenient syntax for piping the expressions directly to another function which linearizes them and results a new result listing the possible linearizations::

    >>> parse('where is the museum', lang='en_US') | linearize(lang='sv_SE')
    <Result:
    0: var är museet ?
    1: var är museet
    >

Since the result object is an iterable, individual linearizations can be accessed by standard means, e.g. by assigning to a target list::

      >>> lin1, lin2 = parse('where is the museum', lang='en_US') | linearize(lang='sv_SE')
      >>> lin1
      'var är museet ?'

For more intricate operations, refer to the full API.

.. _`GF interpreter`: http://www.grammaticalframework.org/doc/gf-shell-reference.html


API Documentation
=================

.. automodule:: pgf
   :members:

.. automodule:: pgf.pygf
   :members:

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`

