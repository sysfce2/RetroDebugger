/*
 * final.c - Cartridge handling, Final cart.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  groepaz <groepaz@gmx.net>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "export.h"
#include "final.h"
#include "monitor.h"
#include "snapshot.h"
#include "vicetypes.h"
#include "util.h"
#include "crt.h"

/*
    The Final Cartridge 1+2

   - 16k ROM

   - any access to IO1 turns cartridge ROM off
   - any access to IO2 turns cartridge ROM on

   - cart ROM mirror is visible in io1/io2
*/

/* #define FCDEBUG */

#ifdef FCDEBUG
#define DBG(x) printf x
#else
#define DBG(x)
#endif

/* some prototypes are needed */
static BYTE final_v1_io1_read(WORD addr);
static BYTE final_v1_io1_peek(WORD addr);
static void final_v1_io1_store(WORD addr, BYTE value);
static BYTE final_v1_io2_read(WORD addr);
static BYTE final_v1_io2_peek(WORD addr);
static void final_v1_io2_store(WORD addr, BYTE value);
static int final_v1_dump(void);

static io_source_t final1_io1_device = {
    CARTRIDGE_NAME_FINAL_I,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    1, /* read is always valid */
    final_v1_io1_store,
    final_v1_io1_read,
    final_v1_io1_peek,
    final_v1_dump,
    CARTRIDGE_FINAL_I,
    0,
    0
};

static io_source_t final1_io2_device = {
    CARTRIDGE_NAME_FINAL_I,
    IO_DETACH_CART,
    NULL,
    0xdf00, 0xdfff, 0xff,
    1, /* read is always valid */
    final_v1_io2_store,
    final_v1_io2_read,
    final_v1_io2_peek,
    final_v1_dump,
    CARTRIDGE_FINAL_I,
    0,
    0
};

static io_source_list_t *final1_io1_list_item = NULL;
static io_source_list_t *final1_io2_list_item = NULL;

static const export_resource_t export_res_v1 = {
    CARTRIDGE_NAME_FINAL_I, 1, 1, &final1_io1_device, &final1_io2_device, CARTRIDGE_FINAL_I
};

/* ---------------------------------------------------------------------*/

static int final_v1_active = 0;

static BYTE final_v1_io1_read(WORD addr)
{
    DBG(("disable %04x\n", addr));
    cart_config_changed_slotmain(2, 2, CMODE_READ | CMODE_RELEASE_FREEZE);
    final_v1_active = 0;
    return roml_banks[0x1e00 + (addr & 0xff)];
}

static BYTE final_v1_io1_peek(WORD addr)
{
    return roml_banks[0x1e00 + (addr & 0xff)];
}

static void final_v1_io1_store(WORD addr, BYTE value)
{
    DBG(("disable %04x %02x\n", addr, value));
    cart_config_changed_slotmain(2, 2, CMODE_WRITE | CMODE_RELEASE_FREEZE);
    final_v1_active = 0;
}

static BYTE final_v1_io2_read(WORD addr)
{
    DBG(("enable %04x\n", addr));
    cart_config_changed_slotmain(1, 1, CMODE_READ | CMODE_RELEASE_FREEZE);
    final_v1_active = 1;
    return roml_banks[0x1f00 + (addr & 0xff)];
}

static BYTE final_v1_io2_peek(WORD addr)
{
    return roml_banks[0x1f00 + (addr & 0xff)];
}

static void final_v1_io2_store(WORD addr, BYTE value)
{
    DBG(("enable %04x %02x\n", addr, value));
    cart_config_changed_slotmain(1, 1, CMODE_WRITE | CMODE_RELEASE_FREEZE);
    final_v1_active = 1;
}

static int final_v1_dump(void)
{
    mon_out("ROM at $8000-$BFFF: %s\n", (final_v1_active) ? "enabled" : "disabled");

    return 0;
}

/* ---------------------------------------------------------------------*/

BYTE final_v1_roml_read(WORD addr)
{
    return roml_banks[(addr & 0x1fff)];
}

BYTE final_v1_romh_read(WORD addr)
{
    return romh_banks[(addr & 0x1fff)];
}

/* ---------------------------------------------------------------------*/

void final_v1_freeze(void)
{
    DBG(("freeze enable\n"));
    cart_config_changed_slotmain(3, 3, CMODE_READ | CMODE_RELEASE_FREEZE);
    final_v1_active = 1;
    cartridge_release_freeze();
}

void final_v1_config_init(void)
{
    cart_config_changed_slotmain(1, 1, CMODE_READ);
    final_v1_active = 1;
}

void final_v1_config_setup(BYTE *rawcart)
{
    memcpy(roml_banks, rawcart, 0x2000);
    memcpy(romh_banks, &rawcart[0x2000], 0x2000);
    cart_config_changed_slotmain(1, 1, CMODE_READ);
    final_v1_active = 1;
}

/* ---------------------------------------------------------------------*/

static int final_v1_common_attach(void)
{
    if (export_add(&export_res_v1) < 0) {
        return -1;
    }

    final1_io1_list_item = io_source_register(&final1_io1_device);
    final1_io2_list_item = io_source_register(&final1_io2_device);

    return 0;
}

int final_v1_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, 0x4000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    return final_v1_common_attach();
}

int final_v1_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;

    if (crt_read_chip_header(&chip, fd)) {
        return -1;
    }

    if (chip.start != 0x8000 || chip.size != 0x4000) {
        return -1;
    }

    if (crt_read_chip(rawcart, 0, &chip, fd)) {
        return -1;
    }

    return final_v1_common_attach();
}

void final_v1_detach(void)
{
    export_remove(&export_res_v1);
    io_source_unregister(final1_io1_list_item);
    io_source_unregister(final1_io2_list_item);
    final1_io1_list_item = NULL;
    final1_io2_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTFINALV1 snapshot module format:

   type  | name   | version | description
   --------------------------------------
   BYTE  | active |   0.1   | cartridge active flag
   ARRAY | ROML   |   0.0+  | 8192 BYTES of ROML data
   ARRAY | ROMH   |   0.0+  | 8192 BYTES of ROMH data
 */

static char snap_module_name[] = "CARTFINALV1";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

int final_v1_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (BYTE)final_v1_active) < 0)
        || (SMW_BA(m, roml_banks, 0x2000) < 0)
        || (SMW_BA(m, romh_banks, 0x2000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int final_v1_snapshot_read_module(snapshot_t *s)
{
    BYTE vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (vmajor > SNAP_MAJOR || vminor > SNAP_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    /* new in 0.1 */
    if (SNAPVAL(vmajor, vminor, 0, 1)) {
        if (SMR_B_INT(m, &final_v1_active) < 0) {
            goto fail;
        }
    } else {
        final_v1_active = 0;
    }

    if (0
        || (SMR_BA(m, roml_banks, 0x2000) < 0)
        || (SMR_BA(m, romh_banks, 0x2000) < 0)) {
        goto fail;
    }

    snapshot_module_close(m);

    return final_v1_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
