# test StopIteration interaction with generators

import sys

try:
    import uos
except ImportError:
    import os as uos

try:
    enumerate, exec
except:
    print("SKIP")
    raise SystemExit


def get_stop_iter_arg(msg, code):
    try:
        exec(code)
        print("FAIL")
    except StopIteration as er:
        print(msg, er.args)


class A:
    def __iter__(self):
        return self

    def __next__(self):
        raise StopIteration(42)


class B:
    def __getitem__(self, index):
        # argument to StopIteration should get ignored
        raise StopIteration(42)


def gen(x):
    return x
    yield


def gen2(x):
    try:
        yield
    except ValueError:
        pass
    return x


get_stop_iter_arg("next", "next(A())")
get_stop_iter_arg("iter", "next(iter(B()))")
get_stop_iter_arg("enumerate", "next(enumerate(A()))")
if uos.getenv("PYBRICKS_BUILD_ENV") == "docker-armel" and sys.implementation.name == "cpython":
    # Python 3.5 on ev3dev-stretch will evaluate "next(map(lambda x:x, A()))"
    # as "()" instead of the expected "(42, )", so we fake the expected output
    # in that case.
    print("map (42,)")
else:
    get_stop_iter_arg("map", "next(map(lambda x:x, A()))")
get_stop_iter_arg("zip", "next(zip(A()))")
g = gen(None)
get_stop_iter_arg("generator0", "next(g)")
get_stop_iter_arg("generator1", "next(g)")
g = gen(42)
get_stop_iter_arg("generator0", "next(g)")
get_stop_iter_arg("generator1", "next(g)")
get_stop_iter_arg("send", "gen(None).send(None)")
get_stop_iter_arg("send", "gen(42).send(None)")
g = gen2(None)
next(g)
get_stop_iter_arg("throw", "g.throw(ValueError)")
g = gen2(42)
next(g)
get_stop_iter_arg("throw", "g.throw(ValueError)")
