#include "py/builtin.h"
#include "py/mperrno.h"
#include "py/lexer.h"
#include "py/runtime.h"

mp_import_stat_t mp_import_stat(const char *path) {
    return MP_IMPORT_STAT_NO_EXIST;
}

mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs)
{
    mp_raise_OSError(MP_EPERM);
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);

mp_lexer_t *mp_lexer_new_from_file(qstr filename)
{
    mp_raise_OSError(MP_ENOENT);
}