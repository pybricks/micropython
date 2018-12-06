/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 * Copyright (c) 2018 David Lechner <david@lechnology.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/types.h>

#include "py/mpconfig.h"
#include "py/mpthread.h"
#include "py/runtime.h"

typedef enum {
    ACCESS_DEFAULT,
    ACCESS_READ,
    ACCESS_WRITE,
    ACCESS_COPY
} mp_module_ummap_access_t;

typedef struct {
    mp_obj_base_t base;
    char *addr;
    size_t len;
    size_t pos;
    mp_module_ummap_access_t access;
} mp_obj_mmap_t;

STATIC const mp_obj_type_t mp_module_ummap_mmap_type;

STATIC mp_obj_t mp_module_ummap_mmap_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_obj_mmap_t *self;
    int prot = PROT_WRITE | PROT_READ;
    int flags = MAP_SHARED;
    int fd;
    size_t offset = 0;

    mp_arg_check_num(n_args, n_kw, 2, 6, false);

    self = m_new_obj_with_finaliser(mp_obj_mmap_t);
    self->base.type = &mp_module_ummap_mmap_type;
    self->access = ACCESS_DEFAULT;

    fd = mp_obj_get_int(args[0]);
    self->len = mp_obj_get_int(args[1]);
    // TODO: needs more arg checking and handle kw args
    if (n_args > 2) {
        flags = mp_obj_get_int(args[2]);
    }
    if (n_args > 3) {
        prot = mp_obj_get_int(args[3]);
    }
    if (n_args > 4) {
        self->access = mp_obj_get_int(args[4]);
    }
    if (n_args > 5) {
        offset = mp_obj_get_int(args[5]);
    }

    MP_THREAD_GIL_EXIT();
    self->addr = mmap(NULL, self->len, prot, flags, fd, offset);
    MP_THREAD_GIL_ENTER();
    if (self->addr == MAP_FAILED) {
        int err = errno;
        mp_raise_OSError(err);
    }

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t mp_module_ummap_mmap_close(mp_obj_t self_in) {
    mp_obj_mmap_t *self = MP_OBJ_TO_PTR(self_in);
    int ret;

    MP_THREAD_GIL_EXIT();
    ret = munmap(self->addr, self->len);
    MP_THREAD_GIL_ENTER();
    if (ret == -1) {
        int err = errno;
        mp_raise_OSError(err);
    }
    self->addr = MAP_FAILED;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_module_ummap_mmap_close_obj, mp_module_ummap_mmap_close);

STATIC mp_obj_t mp_module_ummap_mmap_read(size_t n_args, const mp_obj_t *args) {
    mp_obj_mmap_t *self = MP_OBJ_TO_PTR(args[0]);
    ssize_t size = self->len - self->pos;
    mp_obj_t bytes;

    if (self->addr == MAP_FAILED) {
        mp_raise_ValueError(MP_ERROR_TEXT("mmap is closed"));
    }

    // If the argument is omitted, None or negative, return all bytes from the
    // current file position to the end of the mapping.
    if (n_args > 1 && args[1] != mp_const_none) {
        int size_arg = mp_obj_get_int(args[1]);
        if (size_arg >= 0 && size_arg < size) {
            size = size_arg;
        }
    }

    if (size == 0) {
        return mp_const_none;
    }

    bytes = mp_obj_new_bytearray(size, self->addr + self->pos);
    self->pos += size;

    return bytes;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_module_ummap_mmap_read_obj, 1, 2, mp_module_ummap_mmap_read);

STATIC mp_obj_t mp_module_ummap_mmap_seek(size_t n_args, const mp_obj_t *args) {
    mp_obj_mmap_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t pos = mp_obj_get_int(args[1]);
    int whence = SEEK_SET;

    if (n_args > 2) {
        whence = mp_obj_get_int(args[2]);
    }

    if (self->addr == MAP_FAILED) {
        mp_raise_ValueError(MP_ERROR_TEXT("mmap is closed"));
    }

    switch (whence) {
        case SEEK_SET:
            break;
        case SEEK_CUR:
            pos += self->pos;
            break;
        case SEEK_END:
            pos += self->len;
            break;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("bad whence"));
    }

    if (pos < 0 || pos >= (mp_int_t)self->len) {
        mp_raise_ValueError(MP_ERROR_TEXT("out of range"));
    }
    self->pos = pos;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_module_ummap_mmap_seek_obj, 2, 3, mp_module_ummap_mmap_seek);

STATIC mp_obj_t mp_module_ummap_mmap_write(mp_obj_t self_in, mp_obj_t bytes) {
    mp_obj_mmap_t *self = MP_OBJ_TO_PTR(self_in);
    mp_buffer_info_t bufinfo;

    if (self->addr == MAP_FAILED) {
        mp_raise_ValueError(MP_ERROR_TEXT("mmap is closed"));
    }

    if (self->access == ACCESS_READ) {
        mp_raise_TypeError(MP_ERROR_TEXT("ACCESS_READ"));
    }

    mp_get_buffer_raise(bytes, &bufinfo, MP_BUFFER_READ);

    for (size_t i = 0; i < bufinfo.len && self->pos < self->len; self->pos++, i++) {
        self->addr[self->pos] = ((char *)bufinfo.buf)[i];
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_module_ummap_mmap_write_obj, mp_module_ummap_mmap_write);

STATIC const mp_rom_map_elem_t mp_module_ummap_mmap_locals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&mp_module_ummap_mmap_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_module_ummap_mmap_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_seek), MP_ROM_PTR(&mp_module_ummap_mmap_seek_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_module_ummap_mmap_write_obj) },
};
STATIC MP_DEFINE_CONST_DICT(mp_module_ummap_mmap_locals_dict, mp_module_ummap_mmap_locals_table);

STATIC MP_DEFINE_CONST_OBJ_TYPE(mp_module_ummap_mmap_type,
    MP_QSTR_mmap,
    MP_TYPE_FLAG_NONE,
    make_new, mp_module_ummap_mmap_make_new,
    locals_dict, &mp_module_ummap_mmap_locals_dict);

STATIC const mp_rom_map_elem_t mp_module_ummap_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_mmap) },
    { MP_ROM_QSTR(MP_QSTR_mmap), MP_ROM_PTR(&mp_module_ummap_mmap_type) },
    { MP_ROM_QSTR(MP_QSTR_ACCESS_COPY), MP_ROM_INT(ACCESS_COPY) },
    { MP_ROM_QSTR(MP_QSTR_ACCESS_READ), MP_ROM_INT(ACCESS_READ) },
    { MP_ROM_QSTR(MP_QSTR_ACCESS_WRITE), MP_ROM_INT(ACCESS_WRITE) },
    { MP_ROM_QSTR(MP_QSTR_MAP_PRIVATE), MP_ROM_INT(MAP_PRIVATE) },
    { MP_ROM_QSTR(MP_QSTR_MAP_SHARED), MP_ROM_INT(MAP_SHARED) },
    { MP_ROM_QSTR(MP_QSTR_PROT_EXEC), MP_ROM_INT(PROT_EXEC) },
    { MP_ROM_QSTR(MP_QSTR_PROT_READ), MP_ROM_INT(PROT_READ) },
    { MP_ROM_QSTR(MP_QSTR_PROT_WRITE), MP_ROM_INT(PROT_WRITE) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_ummap_globals, mp_module_ummap_globals_table);

const mp_obj_module_t mp_module_ummap = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_ummap_globals,
};
