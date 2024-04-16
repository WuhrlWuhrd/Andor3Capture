# This file was automatically generated by SWIG (https://www.swig.org).
# Version 4.1.0
#
# Do not make changes to this file unless you know what you are doing - modify
# the SWIG interface file instead.

from sys import version_info as _swig_python_version_info
# Import the low-level C/C++ module
if __package__ or "." in __name__:
    from . import _PyAndor3Capture
else:
    import _PyAndor3Capture

try:
    import builtins as __builtin__
except ImportError:
    import __builtin__

def _swig_repr(self):
    try:
        strthis = "proxy of " + self.this.__repr__()
    except __builtin__.Exception:
        strthis = ""
    return "<%s.%s; %s >" % (self.__class__.__module__, self.__class__.__name__, strthis,)


def _swig_setattr_nondynamic_instance_variable(set):
    def set_instance_attr(self, name, value):
        if name == "this":
            set(self, name, value)
        elif name == "thisown":
            self.this.own(value)
        elif hasattr(self, name) and isinstance(getattr(type(self), name), property):
            set(self, name, value)
        else:
            raise AttributeError("You cannot add instance attributes to %s" % self)
    return set_instance_attr


def _swig_setattr_nondynamic_class_variable(set):
    def set_class_attr(cls, name, value):
        if hasattr(cls, name) and not isinstance(getattr(cls, name), property):
            set(cls, name, value)
        else:
            raise AttributeError("You cannot add class attributes to %s" % cls)
    return set_class_attr


def _swig_add_metaclass(metaclass):
    """Class decorator for adding a metaclass to a SWIG wrapped class - a slimmed down version of six.add_metaclass"""
    def wrapper(cls):
        return metaclass(cls.__name__, cls.__bases__, cls.__dict__.copy())
    return wrapper


class _SwigNonDynamicMeta(type):
    """Meta class to enforce nondynamic attributes (no new attributes) for a class"""
    __setattr__ = _swig_setattr_nondynamic_class_variable(type.__setattr__)


class A3C(object):
    thisown = property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc="The membership flag")
    __repr__ = _swig_repr

    def __init__(self, handle):
        _PyAndor3Capture.A3C_swiginit(self, _PyAndor3Capture.new_A3C(handle))

    def setFrameLimit(self, limit):
        return _PyAndor3Capture.A3C_setFrameLimit(self, limit)

    def setOutputPath(self, path):
        return _PyAndor3Capture.A3C_setOutputPath(self, path)

    def getFrameLimit(self):
        return _PyAndor3Capture.A3C_getFrameLimit(self)

    def getOutputPath(self):
        return _PyAndor3Capture.A3C_getOutputPath(self)

    def start(self):
        return _PyAndor3Capture.A3C_start(self)

    def stop(self):
        return _PyAndor3Capture.A3C_stop(self)

    def acquire(self):
        return _PyAndor3Capture.A3C_acquire(self)

    def process(self):
        return _PyAndor3Capture.A3C_process(self)

    def write(self):
        return _PyAndor3Capture.A3C_write(self)
    __swig_destroy__ = _PyAndor3Capture.delete_A3C

# Register A3C in _PyAndor3Capture:
_PyAndor3Capture.A3C_swigregister(A3C)

