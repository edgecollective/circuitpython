/*
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Craig Versek and Don Blair for EdgeCollective
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
 #include "py/obj.h"
 #include "py/runtime.h"
 #include "sleepydog.h"

//| :mod:`sleepydog` --- lower power sleep mode using WDT
//| =================================================
//|
//| .. module:: sleepydog
//|   :synopsis: lower power sleep mode using WDT
//|   :platform: SAMD21
//|

//| .. method:: sleepydog_test()
//|
//|
STATIC mp_obj_t modsleepydog_sleep(mp_obj_t seconds_o) {
    #if MICROPY_PY_BUILTINS_FLOAT
    float seconds = mp_obj_get_float(seconds_o);
    #else
    int seconds = mp_obj_get_int(seconds_o);
    #endif
    if (seconds < 0) {
        mp_raise_ValueError("sleep length must be non-negative");
    }
    sleepydog_sleep(seconds*1000);
    //sleepydog_reset();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(modsleepydog_sleep_obj, modsleepydog_sleep);


STATIC const mp_rom_map_elem_t sleepydog_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_sleepydog) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_sleep),  MP_ROM_PTR(&modsleepydog_sleep_obj)},
};

STATIC MP_DEFINE_CONST_DICT(sleepydog_module_globals, sleepydog_module_globals_table);

const mp_obj_module_t sleepydog_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&sleepydog_module_globals,
};
