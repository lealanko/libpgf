from ctypes import *
import ctypes.util


def _load_dladdr():
    for libname in ['dl', 'c']:
        soname = ctypes.util.find_library(libname)
        if soname is None:
            continue
        try:
            dll = CDLL(soname)
        except OSError:
            continue
        try:
            dladdr = dll.dladdr
        except AttributeError:
            continue
        return dladdr
    return None

_dladdr = _load_dladdr()
if _dladdr is None:
    def dladdr(addr):
        return None
    
else:
    class Dl_info(Structure):
        _fields_ = [
            ('dli_fname', c_char_p),
            ('dli_fbase', c_void_p),
            ('dli_sname', c_char_p),
            ('dli_saddr', c_void_p)
            ]
    _dladdr.argtypes = [c_void_p, POINTER(Dl_info)]

    def dladdr(addr):
        info = Dl_info()
        ret = _dladdr(addr, byref(info))
        if not ret:
            return None
        if info.dli_saddr != addr.value:
            return None
        return info.dli_sname.decode()

