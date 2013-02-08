from .core import *

if sizeof(c_wchar) >= sizeof(c_int32):
    UCS = c_wchar
else:
    UCS = c_int32

    
