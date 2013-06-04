from setuptools import setup

setup(
    name = 'PyGF',
    version = '0.3',
    packages = ['gupy', 'gu', 'pgf'],
    license = 'GNU Lesser General Public License version 3',
    author = 'Lauri Alanko',
    author_email = 'lealanko@ling.helsinki.fi',
    include_package_data = True,
    package_data = {
        'docs': ['conf.py', '*.rst'],
        },
    long_description=open('README.rst').read(),
)
