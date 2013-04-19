import inspect
from gupy.ffi import CFuncPtrBase
from sphinx.ext.autodoc import (
    FunctionDocumenter, AutoDirective, ClassLevelDocumenter, ModuleDocumenter
    )

def format_signature(sig, delete_self=False):
    def format_param(param):
        fmt = param.name
        if param.default is not param.empty:
            fmt = '%s = %r' % (fmt, param.default)
        elif param.annotation is not param.empty:
            if getattr(param.annotation, 'optional', False):
                fmt += '=None'
        return fmt
    params = list(sig.parameters.values())
    if delete_self:
        del params[0]
    return '(' + ', '.join(map(format_param, params)) + ')'


class CMethodDocumenter(ClassLevelDocumenter):
    """
    Specialized Documenter subclass for methods (normal, static and class).
    """
    objtype = 'cmethod'
    member_order = 50
    priority = 20  # must be more than AttributeDocumenter

    @classmethod
    def can_document_member(cls, member, membername, isattr, parent):
        return (isinstance(member, CFuncPtrBase)
                and not isinstance(parent, ModuleDocumenter))

    def import_object(self):
        ret = ClassLevelDocumenter.import_object(self)
        if self.object.static:
            self.directivetype = 'staticmethod'
            self.member_order = self.member_order - 1
        else:
            self.directivetype = 'method'
        return ret

    def format_args(self):
        try:
            sig = inspect.signature(self.object)
        except TypeError:
            return None
        else:
            return format_signature(sig, not self.object.static)

    def document_members(self, all_members=False):
        pass


def setup(app):
    app.add_autodocumenter(CMethodDocumenter)
