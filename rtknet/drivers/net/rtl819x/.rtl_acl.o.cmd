cmd_drivers/net/rtl819x/rtl_acl.o := msdk-linux-gcc -Wp,-MD,drivers/net/rtl819x/.rtl_acl.o.d  -nostdinc -isystem /opt/workspace/3.4.14/rtl819x/toolchain/msdk-4.8.5-mips-EL-3.10-u0.9.33-m32ut-180418/bin/../lib/gcc/mips-linux-uclibc/4.8.5/include -I/opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include -Iarch/mips-ori/include/generated  -Iinclude -I/opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/uapi -Iarch/mips-ori/include/generated/uapi -I/opt/workspace/3.4.14/rtl819x/linux-3.10/include/uapi -Iinclude/generated/uapi -include /opt/workspace/3.4.14/rtl819x/linux-3.10/include/linux/kconfig.h -D__KERNEL__ -DVMLINUX_LOAD_ADDRESS=0x80000000 -DDATAOFFSET=0 -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Wno-format-security -fno-delete-null-pointer-checks -std=gnu89 -O2 -mno-check-zero-division -mabi=32 -G 0 -mno-abicalls -fno-pic -pipe -msoft-float -ffreestanding -EL -UMIPSEB -U_MIPSEB -U__MIPSEB -U__MIPSEB__ -UMIPSEL -U_MIPSEL -U__MIPSEL -U__MIPSEL__ -DMIPSEL -D_MIPSEL -D__MIPSEL -D__MIPSEL__ -march=mips32r2 -Wa,-mips32r2 -Wa,--trap -I/opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/bsp -I/opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/mach-generic -Wframe-larger-than=1024 -fno-stack-protector -Wno-unused-but-set-variable -fomit-frame-pointer -fno-var-tracking-assignments -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fconserve-stack -DCC_HAVE_ASM_GOTO -DCONFIG_RTL865X_MODULE_INTERNAL -DRTL865X_OVER_KERNEL -DRTL865X_OVER_LINUX -D__KERNEL__ -Wno-implicit -I/opt/workspace/3.4.14/rtl819x/linux-3.10/include -I/opt/workspace/3.4.14/rtl819x/target/bsp -I/opt/workspace/3.4.14/rtl819x/linux-3.10/drivers/net/rtl819x/ -I/opt/workspace/3.4.14/rtl819x/target -I/opt/workspace/3.4.14/rtl819x/linux-3.10/drivers/net/rtl819x//rtl83xx -D__KERNEL__ -DEXPORT_SYMTAB    -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(rtl_acl)"  -D"KBUILD_MODNAME=KBUILD_STR(rtl_acl)" -c -o drivers/net/rtl819x/rtl_acl.o drivers/net/rtl819x/rtl_acl.c

source_drivers/net/rtl819x/rtl_acl.o := drivers/net/rtl819x/rtl_acl.c

deps_drivers/net/rtl819x/rtl_acl.o := \
    $(wildcard include/config/rtl/proc/new.h) \
    $(wildcard include/config/rtl/83xx/acl/support.h) \
    $(wildcard include/config/rtl/8367r/support.h) \
    $(wildcard include/config/rtl/83xx/support.h) \
  include/generated/uapi/linux/version.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/uaccess.h \
    $(wildcard include/config/32bit.h) \
    $(wildcard include/config/kvm/guest.h) \
    $(wildcard include/config/64bit.h) \
    $(wildcard include/config/cpu/daddi/workarounds.h) \
  include/linux/kernel.h \
    $(wildcard include/config/lbdaf.h) \
    $(wildcard include/config/preempt/voluntary.h) \
    $(wildcard include/config/debug/atomic/sleep.h) \
    $(wildcard include/config/prove/locking.h) \
    $(wildcard include/config/ring/buffer.h) \
    $(wildcard include/config/tracing.h) \
    $(wildcard include/config/ftrace/mcount/record.h) \
  /opt/workspace/3.4.14/rtl819x/toolchain/msdk-4.8.5-mips-EL-3.10-u0.9.33-m32ut-180418/lib/gcc/mips-linux-uclibc/4.8.5/include/stdarg.h \
  include/linux/linkage.h \
  include/linux/compiler.h \
    $(wildcard include/config/sparse/rcu/pointer.h) \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
    $(wildcard include/config/kprobes.h) \
  include/linux/compiler-gcc.h \
    $(wildcard include/config/arch/supports/optimized/inlining.h) \
    $(wildcard include/config/optimize/inlining.h) \
  include/linux/compiler-gcc4.h \
    $(wildcard include/config/arch/use/builtin/bswap.h) \
  include/linux/stringify.h \
  include/linux/export.h \
    $(wildcard include/config/have/underscore/symbol/prefix.h) \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/modversions.h) \
    $(wildcard include/config/unused/symbols.h) \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/linkage.h \
  include/linux/stddef.h \
  include/uapi/linux/stddef.h \
  include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/arch/dma/addr/t/64bit.h) \
    $(wildcard include/config/phys/addr/t/64bit.h) \
  include/uapi/linux/types.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/types.h \
    $(wildcard include/config/64bit/phys/addr.h) \
  include/asm-generic/int-ll64.h \
  include/uapi/asm-generic/int-ll64.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/uapi/asm/bitsperlong.h \
  include/asm-generic/bitsperlong.h \
  include/uapi/asm-generic/bitsperlong.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/uapi/asm/types.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/include/uapi/linux/posix_types.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/uapi/asm/posix_types.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/uapi/asm/sgidefs.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/include/uapi/asm-generic/posix_types.h \
  include/linux/bitops.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/bitops.h \
    $(wildcard include/config/cpu/mipsr2.h) \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/barrier.h \
    $(wildcard include/config/cpu/has/sync.h) \
    $(wildcard include/config/cpu/cavium/octeon.h) \
    $(wildcard include/config/sgi/ip28.h) \
    $(wildcard include/config/cpu/has/wb.h) \
    $(wildcard include/config/weak/ordering.h) \
    $(wildcard include/config/smp.h) \
    $(wildcard include/config/weak/reordering/beyond/llsc.h) \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/addrspace.h \
    $(wildcard include/config/cpu/r8000.h) \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/mach-generic/spaces.h \
    $(wildcard include/config/dma/noncoherent.h) \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/include/uapi/linux/const.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/uapi/asm/byteorder.h \
  include/linux/byteorder/little_endian.h \
  include/uapi/linux/byteorder/little_endian.h \
  include/linux/swab.h \
  include/uapi/linux/swab.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/uapi/asm/swab.h \
  include/linux/byteorder/generic.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/cpu-features.h \
    $(wildcard include/config/cpu/mipsr2/irq/vi.h) \
    $(wildcard include/config/cpu/mipsr2/irq/ei.h) \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/cpu.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/cpu-info.h \
    $(wildcard include/config/mips/mt/smp.h) \
    $(wildcard include/config/mips/mt/smtc.h) \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/cache.h \
    $(wildcard include/config/mips/l1/cache/shift.h) \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/mach-generic/kmalloc.h \
    $(wildcard include/config/dma/coherent.h) \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/bsp/cpu-feature-overrides.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/war.h \
    $(wildcard include/config/cpu/r4000/workarounds.h) \
    $(wildcard include/config/cpu/r4400/workarounds.h) \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/bsp/war.h \
  include/asm-generic/bitops/non-atomic.h \
  include/asm-generic/bitops/fls64.h \
  include/asm-generic/bitops/ffz.h \
  include/asm-generic/bitops/find.h \
    $(wildcard include/config/generic/find/first/bit.h) \
  include/asm-generic/bitops/sched.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/arch_hweight.h \
  include/asm-generic/bitops/arch_hweight.h \
  include/asm-generic/bitops/const_hweight.h \
  include/asm-generic/bitops/le.h \
  include/asm-generic/bitops/ext2-atomic.h \
  include/linux/log2.h \
    $(wildcard include/config/arch/has/ilog2/u32.h) \
    $(wildcard include/config/arch/has/ilog2/u64.h) \
  include/linux/typecheck.h \
  include/linux/printk.h \
    $(wildcard include/config/early/printk.h) \
    $(wildcard include/config/printk.h) \
    $(wildcard include/config/printk/func.h) \
    $(wildcard include/config/dynamic/debug.h) \
  include/linux/init.h \
    $(wildcard include/config/broken/rodata.h) \
  include/linux/kern_levels.h \
  include/linux/dynamic_debug.h \
  include/linux/string.h \
    $(wildcard include/config/binary/printf.h) \
  include/uapi/linux/string.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/string.h \
    $(wildcard include/config/cpu/r3000.h) \
  include/linux/errno.h \
  include/uapi/linux/errno.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/errno.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/uapi/asm/errno.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/include/uapi/asm-generic/errno-base.h \
  include/uapi/linux/kernel.h \
    $(wildcard include/config/rlx.h) \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/include/uapi/linux/sysinfo.h \
    $(wildcard include/config/rtl/819x.h) \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/div64.h \
  include/asm-generic/div64.h \
  include/linux/thread_info.h \
    $(wildcard include/config/compat.h) \
    $(wildcard include/config/debug/stack/usage.h) \
  include/linux/bug.h \
    $(wildcard include/config/generic/bug.h) \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/bug.h \
    $(wildcard include/config/bug.h) \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/break.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/uapi/asm/break.h \
  include/asm-generic/bug.h \
    $(wildcard include/config/generic/bug/relative/pointers.h) \
    $(wildcard include/config/debug/bugverbose.h) \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/thread_info.h \
    $(wildcard include/config/page/size/4kb.h) \
    $(wildcard include/config/page/size/8kb.h) \
    $(wildcard include/config/page/size/16kb.h) \
    $(wildcard include/config/page/size/32kb.h) \
    $(wildcard include/config/page/size/64kb.h) \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/processor.h \
    $(wildcard include/config/cavium/octeon/cvmseg/size.h) \
    $(wildcard include/config/mips/mt/fpaff.h) \
    $(wildcard include/config/cpu/has/prefetch.h) \
  include/linux/cpumask.h \
    $(wildcard include/config/cpumask/offstack.h) \
    $(wildcard include/config/hotplug/cpu.h) \
    $(wildcard include/config/debug/per/cpu/maps.h) \
    $(wildcard include/config/disable/obsolete/cpumask/functions.h) \
  include/linux/threads.h \
    $(wildcard include/config/nr/cpus.h) \
    $(wildcard include/config/base/small.h) \
  include/linux/bitmap.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/uapi/asm/cachectl.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/mipsregs.h \
    $(wildcard include/config/cpu/vr41xx.h) \
    $(wildcard include/config/mips/huge/tlb/support.h) \
    $(wildcard include/config/cpu/micromips.h) \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/hazards.h \
    $(wildcard include/config/cpu/mipsr1.h) \
    $(wildcard include/config/mips/alchemy.h) \
    $(wildcard include/config/cpu/bmips.h) \
    $(wildcard include/config/cpu/loongson2.h) \
    $(wildcard include/config/cpu/r10000.h) \
    $(wildcard include/config/cpu/r5500.h) \
    $(wildcard include/config/cpu/xlr.h) \
    $(wildcard include/config/cpu/sb1.h) \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/prefetch.h \
  drivers/net/rtl819x/rtl83xx/rtk_types.h \
  drivers/net/rtl819x/rtl83xx/rtl8367c_asicdrv_acl.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/drivers/net/rtl819x//rtl83xx/rtl8367c_asicdrv.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/drivers/net/rtl819x//rtl83xx/rtk_types.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/drivers/net/rtl819x//rtl83xx/rtk_error.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/drivers/net/rtl819x//rtl83xx/rtl8367c_reg.h \
    $(wildcard include/config/rst/offset.h) \
    $(wildcard include/config/rst/mask.h) \
    $(wildcard include/config/dummy/15/offset.h) \
    $(wildcard include/config/dummy/15/mask.h) \
    $(wildcard include/config/sel/offset.h) \
    $(wildcard include/config/sel/mask.h) \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/drivers/net/rtl819x//rtl83xx/rtl8367c_base.h \
  drivers/net/rtl819x/rtl83xx/rtk_api.h \
    $(wildcard include/config/rtl/custom/passthru.h) \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/drivers/net/rtl819x//rtl83xx/port.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/drivers/net/rtl819x//rtl83xx/qos.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/drivers/net/rtl819x//rtl83xx/acl.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/drivers/net/rtl819x//rtl83xx/storm.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/drivers/net/rtl819x//rtl83xx/cpu.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/drivers/net/rtl819x//rtl83xx/dot1x.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/drivers/net/rtl819x//rtl83xx/eee.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/drivers/net/rtl819x//rtl83xx/igmp.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/drivers/net/rtl819x//rtl83xx/interrupt.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/drivers/net/rtl819x//rtl83xx/l2.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/drivers/net/rtl819x//rtl83xx/leaky.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/drivers/net/rtl819x//rtl83xx/led.h \
    $(wildcard include/config/ledoff.h) \
    $(wildcard include/config/dupcol.h) \
    $(wildcard include/config/link/act.h) \
    $(wildcard include/config/spd1000.h) \
    $(wildcard include/config/spd100.h) \
    $(wildcard include/config/spd10.h) \
    $(wildcard include/config/spd1000act.h) \
    $(wildcard include/config/spd100act.h) \
    $(wildcard include/config/spd10act.h) \
    $(wildcard include/config/spd10010act.h) \
    $(wildcard include/config/loopdetect.h) \
    $(wildcard include/config/eee.h) \
    $(wildcard include/config/linkrx.h) \
    $(wildcard include/config/linktx.h) \
    $(wildcard include/config/master.h) \
    $(wildcard include/config/act.h) \
    $(wildcard include/config/end.h) \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/drivers/net/rtl819x//rtl83xx/mirror.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/drivers/net/rtl819x//rtl83xx/oam.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/drivers/net/rtl819x//rtl83xx/ptp.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/drivers/net/rtl819x//rtl83xx/rate.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/drivers/net/rtl819x//rtl83xx/rldp.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/drivers/net/rtl819x//rtl83xx/stat.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/drivers/net/rtl819x//rtl83xx/trap.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/drivers/net/rtl819x//rtl83xx/trunk.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/drivers/net/rtl819x//rtl83xx/vlan.h \
  include/net/rtl/rtl_acl.h \
    $(wildcard include/config/rtl/support/dual/wan/ports.h) \
  include/net/rtl/rtl_queue.h \
  include/net/rtl/rtl_glue.h \
  include/linux/semaphore.h \
  include/linux/list.h \
    $(wildcard include/config/debug/list.h) \
  include/linux/poison.h \
    $(wildcard include/config/illegal/pointer/value.h) \
  include/linux/spinlock.h \
    $(wildcard include/config/debug/spinlock.h) \
    $(wildcard include/config/generic/lockbreak.h) \
    $(wildcard include/config/preempt.h) \
    $(wildcard include/config/debug/lock/alloc.h) \
  include/linux/preempt.h \
    $(wildcard include/config/debug/preempt.h) \
    $(wildcard include/config/preempt/tracer.h) \
    $(wildcard include/config/context/tracking.h) \
    $(wildcard include/config/preempt/count.h) \
    $(wildcard include/config/preempt/notifiers.h) \
  include/linux/irqflags.h \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/rtl/debug/counter.h) \
    $(wildcard include/config/irqsoff/tracer.h) \
    $(wildcard include/config/trace/irqflags/support.h) \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/irqflags.h \
    $(wildcard include/config/irq/cpu.h) \
  include/linux/bottom_half.h \
  include/linux/spinlock_types.h \
  include/linux/spinlock_types_up.h \
  include/linux/lockdep.h \
    $(wildcard include/config/lockdep.h) \
    $(wildcard include/config/lock/stat.h) \
    $(wildcard include/config/prove/rcu.h) \
  include/linux/rwlock_types.h \
  include/linux/spinlock_up.h \
  include/linux/rwlock.h \
  include/linux/spinlock_api_up.h \
  include/linux/atomic.h \
    $(wildcard include/config/arch/has/atomic/or.h) \
    $(wildcard include/config/generic/atomic64.h) \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/atomic.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/cmpxchg.h \
  include/asm-generic/cmpxchg-local.h \
  include/asm-generic/atomic-long.h \
  include/asm-generic/atomic64.h \
  include/linux/slab.h \
    $(wildcard include/config/slab/debug.h) \
    $(wildcard include/config/debug/objects.h) \
    $(wildcard include/config/kmemcheck.h) \
    $(wildcard include/config/failslab.h) \
    $(wildcard include/config/slob.h) \
    $(wildcard include/config/slab.h) \
    $(wildcard include/config/zone/dma.h) \
    $(wildcard include/config/slub.h) \
    $(wildcard include/config/numa.h) \
    $(wildcard include/config/debug/slab.h) \
  include/linux/gfp.h \
    $(wildcard include/config/highmem.h) \
    $(wildcard include/config/zone/dma32.h) \
    $(wildcard include/config/pm/sleep.h) \
    $(wildcard include/config/cma.h) \
  include/linux/mmzone.h \
    $(wildcard include/config/force/max/zoneorder.h) \
    $(wildcard include/config/memory/isolation.h) \
    $(wildcard include/config/memcg.h) \
    $(wildcard include/config/compaction.h) \
    $(wildcard include/config/memory/hotplug.h) \
    $(wildcard include/config/sparsemem.h) \
    $(wildcard include/config/have/memblock/node/map.h) \
    $(wildcard include/config/discontigmem.h) \
    $(wildcard include/config/flat/node/mem/map.h) \
    $(wildcard include/config/no/bootmem.h) \
    $(wildcard include/config/numa/balancing.h) \
    $(wildcard include/config/have/memory/present.h) \
    $(wildcard include/config/have/memoryless/nodes.h) \
    $(wildcard include/config/need/node/memmap/size.h) \
    $(wildcard include/config/need/multiple/nodes.h) \
    $(wildcard include/config/have/arch/early/pfn/to/nid.h) \
    $(wildcard include/config/flatmem.h) \
    $(wildcard include/config/sparsemem/extreme.h) \
    $(wildcard include/config/have/arch/pfn/valid.h) \
    $(wildcard include/config/nodes/span/other/nodes.h) \
    $(wildcard include/config/holes/in/zone.h) \
    $(wildcard include/config/arch/has/holes/memorymodel.h) \
  include/linux/wait.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/current.h \
  include/asm-generic/current.h \
  include/uapi/linux/wait.h \
  include/linux/cache.h \
    $(wildcard include/config/arch/has/cache/line/size.h) \
  include/linux/numa.h \
    $(wildcard include/config/nodes/shift.h) \
  include/linux/seqlock.h \
  include/linux/nodemask.h \
    $(wildcard include/config/movable/node.h) \
  include/linux/pageblock-flags.h \
    $(wildcard include/config/hugetlb/page.h) \
    $(wildcard include/config/hugetlb/page/size/variable.h) \
  include/linux/page-flags-layout.h \
    $(wildcard include/config/sparsemem/vmemmap.h) \
  include/generated/bounds.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/page.h \
    $(wildcard include/config/cpu/mips32.h) \
  include/linux/pfn.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/io.h \
    $(wildcard include/config/pci.h) \
  include/asm-generic/iomap.h \
    $(wildcard include/config/has/ioport.h) \
    $(wildcard include/config/generic/iomap.h) \
  include/asm-generic/pci_iomap.h \
    $(wildcard include/config/no/generic/pci/ioport/map.h) \
    $(wildcard include/config/generic/pci/iomap.h) \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/pgtable-bits.h \
    $(wildcard include/config/cpu/tx39xx.h) \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/mach-generic/ioremap.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/mach-generic/mangle-port.h \
    $(wildcard include/config/swap/io/space.h) \
  include/asm-generic/memory_model.h \
  include/asm-generic/getorder.h \
  include/linux/memory_hotplug.h \
    $(wildcard include/config/memory/hotremove.h) \
    $(wildcard include/config/have/arch/nodedata/extension.h) \
    $(wildcard include/config/have/bootmem/info/node.h) \
  include/linux/notifier.h \
  include/linux/mutex.h \
    $(wildcard include/config/debug/mutexes.h) \
    $(wildcard include/config/mutex/spin/on/owner.h) \
    $(wildcard include/config/have/arch/mutex/cpu/relax.h) \
  include/linux/rwsem.h \
    $(wildcard include/config/rwsem/generic/spinlock.h) \
  include/linux/rwsem-spinlock.h \
  include/linux/srcu.h \
  include/linux/rcupdate.h \
    $(wildcard include/config/rcu/torture/test.h) \
    $(wildcard include/config/tree/rcu.h) \
    $(wildcard include/config/tree/preempt/rcu.h) \
    $(wildcard include/config/rcu/trace.h) \
    $(wildcard include/config/preempt/rcu.h) \
    $(wildcard include/config/rcu/user/qs.h) \
    $(wildcard include/config/tiny/rcu.h) \
    $(wildcard include/config/tiny/preempt/rcu.h) \
    $(wildcard include/config/debug/objects/rcu/head.h) \
    $(wildcard include/config/rcu/nocb/cpu.h) \
  include/linux/completion.h \
  include/linux/debugobjects.h \
    $(wildcard include/config/debug/objects/free.h) \
  include/linux/rcutiny.h \
  include/linux/workqueue.h \
    $(wildcard include/config/debug/objects/work.h) \
    $(wildcard include/config/freezer.h) \
    $(wildcard include/config/sysfs.h) \
  include/linux/timer.h \
    $(wildcard include/config/timer/stats.h) \
    $(wildcard include/config/debug/objects/timers.h) \
  include/linux/ktime.h \
    $(wildcard include/config/ktime/scalar.h) \
  include/linux/time.h \
    $(wildcard include/config/arch/uses/gettimeoffset.h) \
  include/linux/math64.h \
  include/uapi/linux/time.h \
  include/linux/jiffies.h \
  include/linux/timex.h \
  include/uapi/linux/timex.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/include/uapi/linux/param.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/uapi/asm/param.h \
  include/asm-generic/param.h \
    $(wildcard include/config/hz.h) \
  include/uapi/asm-generic/param.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/timex.h \
  include/linux/topology.h \
    $(wildcard include/config/sched/smt.h) \
    $(wildcard include/config/sched/mc.h) \
    $(wildcard include/config/sched/book.h) \
    $(wildcard include/config/use/percpu/numa/node/id.h) \
  include/linux/smp.h \
    $(wildcard include/config/use/generic/smp/helpers.h) \
  include/linux/percpu.h \
    $(wildcard include/config/need/per/cpu/embed/first/chunk.h) \
    $(wildcard include/config/need/per/cpu/page/first/chunk.h) \
    $(wildcard include/config/have/setup/per/cpu/area.h) \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/percpu.h \
  include/asm-generic/percpu.h \
  include/linux/percpu-defs.h \
    $(wildcard include/config/debug/force/weak/per/cpu.h) \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/topology.h \
  /opt/workspace/3.4.14/rtl819x/linux-3.10/arch/mips-ori/include/asm/mach-generic/topology.h \
  include/asm-generic/topology.h \
  include/linux/mmdebug.h \
    $(wildcard include/config/debug/vm.h) \
    $(wildcard include/config/debug/virtual.h) \
  include/linux/slub_def.h \
    $(wildcard include/config/slub/stats.h) \
    $(wildcard include/config/memcg/kmem.h) \
    $(wildcard include/config/slub/debug.h) \
  include/linux/kobject.h \
  include/linux/sysfs.h \
  include/linux/kobject_ns.h \
  include/linux/kref.h \
  include/linux/kmemleak.h \
    $(wildcard include/config/debug/kmemleak.h) \

drivers/net/rtl819x/rtl_acl.o: $(deps_drivers/net/rtl819x/rtl_acl.o)

$(deps_drivers/net/rtl819x/rtl_acl.o):
