#ifndef XNUBOOT_H
#define XNUBOOT_H

#include "qemu/osdep.h"
#include "qemu/datadir.h"
#include "qapi/error.h"
#include "target/arm/cpu-qom.h"
#include "qemu/notify.h"

#define ONE_KB        BIT(10)
#define ONE_MB        BIT(20)
#define ONE_GB        BIT(30)

#define DARWIN_PAGE_SIZE_KB           (( 16 ))
#define DARWIN_HUGE_PAGE_SIZE_MB      (( 32 ))

#define DARWIN_PAGE_SIZE              (( DARWIN_PAGE_SIZE_KB * ONE_KB ))
#define DARWIN_HUGE_PAGE_SIZE         (( DARWIN_HUGE_PAGE_SIZE_MB * ONE_MB ))

#define ROUND_DOWN_POW2(x,sz)      ((       ((x))        & (~(((sz)) - 1)) ))
#define ROUND_UP_POW2(x,sz)        (( (((x)) + (((sz))-1)) & (~(((sz)) - 1)) ))
#define ROUND_NEXT_PAGE(x)         (( ROUND_UP_POW2( ((x)), ( (( DARWIN_PAGE_SIZE )) )) ))
#define XNU_STRIP_VMA(x)           (( ((x)) & ((BIT(35)-1)) ))

typedef uint64_t usize;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t  i8;

typedef struct {
    void   *buf;
    size_t  len;
} mmap_file_t;

struct xnu_boot_info {
    // Arguments to a -M darwin machine
    char *bootkc, *args, *dtree, *sptm, *txm, *tc, *ramdisk;
    mmap_file_t bootkc_f, dtree_f, sptm_f, txm_f, tc_f, ramdisk_f;

    uint64_t dram_base, dram_size;
    bool has_mte;

    // Feedback from the kernel loader back to the darwin machine to boot during reset
    hwaddr  init_pc, init_x0;
};

#define MACHINE_CLASS_ARG(a) \
static char *a##_darwin_class_get(Object *obj, Error **errp) { \
    DarwinState *s = DARWIN_MACHINE(obj); \
    return g_strdup(s->bootinfo.a); \
} \
static void a##_darwin_class_set(Object *obj, const char *path, Error **errp) { \
    DarwinState *s = DARWIN_MACHINE(obj); \
    g_free(s->bootinfo.a); \
    s->bootinfo.a = g_strdup(path); \
}

// Which exception level to hand XNU.
//
// SPTM runs at EL2 and performs the handoff to XNU itself, so an SPTM boot has
// to start at EL2. Without SPTM, iBoot enters XNU at EL1 on real hardware, and
// the kernel keys off CurrentEL: entered at EL2 it takes the VHE path and uses
// the _EL12/_EL02 register aliases, which are UNDEFINED unless HCR_EL2.E2H is
// set. On t8020 that traps in a __pinst thunk before VBAR_EL2 is installed, so
// the vector at PC 0x200 is unmapped, the prefetch abort re-vectors to 0x200
// forever, and the boot is silent on every iOS version from 12.4 to 18.7.11.
//
// TrungNguyen1909/qemu-t8030 boots A13 the same way (has_el2 = false).
#define XNU_WANTS_EL2(info)      (( NULL != ((info))->sptm ))

void arm_load_xnu(ARMCPU *cpu, MachineState *ms, struct xnu_boot_info *info);

#endif /* XNUBOOT_H */
