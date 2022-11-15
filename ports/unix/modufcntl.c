/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 David Lechner <david@lechnology.com>
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

#include "py/mpconfig.h"

#if MICROPY_PY_UFCNTL_POSIX

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>

#include <sys/ioctl.h>

#include "py/misc.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/mpthread.h"


STATIC mp_obj_t mod_ufcntl_ioctl(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    STATIC const mp_arg_t allowed_args[] = {
        { MP_QSTR_fd,          MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_request,     MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_arg,         MP_ARG_OBJ,                   {.u_obj = MP_OBJ_NEW_SMALL_INT(0)} },
        { MP_QSTR_mutate_flag, MP_ARG_BOOL,                  {.u_int = 1} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_int_t fd = args[0].u_int;
    mp_int_t request = args[1].u_int;
    void *arg = NULL;
    bool mutate_flag = args[3].u_bool;

    mp_int_t value;
    if (mp_obj_get_int_maybe(args[2].u_obj, &value)) {
        arg = (void *)value;
    } else {
        mp_buffer_info_t info;
        mp_uint_t flags = mutate_flag ? MP_BUFFER_RW : MP_BUFFER_READ;
        mp_get_buffer_raise(args[2].u_obj, &info, flags);
        arg = info.buf;
    }

    MP_THREAD_GIL_EXIT();
    int ret = ioctl(fd, request, arg);
    MP_THREAD_GIL_ENTER();
    if (ret == -1) {
        mp_raise_OSError(errno);
    }

    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mod_ufcntl_ioctl_obj, 2, mod_ufcntl_ioctl);

STATIC const mp_rom_map_elem_t mp_module_ufcntl_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_ufcntl) },
    { MP_ROM_QSTR(MP_QSTR_ioctl), MP_ROM_PTR(&mod_ufcntl_ioctl_obj) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_ufcntl_globals, mp_module_ufcntl_globals_table);

const mp_obj_module_t mp_module_ufcntl = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_ufcntl_globals,
};

#endif // MICROPY_PY_UFCNTL_POSIX
